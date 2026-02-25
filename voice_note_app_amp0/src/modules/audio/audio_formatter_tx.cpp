/**
 * @file audio_formatter_tx.cpp
 * @brief AudioFormatterTxの実装
 */

// 自ヘッダー
#include "audio_formatter_tx.h"

// 標準ライブラリ
#include <cstring>

// Vitisライブラリ
#include "sleep.h"
#include "xstatus.h"

// プロジェクトライブラリ
#include "logger_core.h"
#include "utility.h"

namespace core0 {
namespace module {

/**
 * @brief IOC割り込みハンドラエントリ
 *
 * @param[in] ref XAudioFormatter*
 */
void AudioFormatterTx::Mm2sIocCallback(void *ref) noexcept
{
    auto *af   = static_cast<XAudioFormatter *>(ref);
    auto *self = static_cast<AudioFormatterTx *>(af->MM2SIOCCallbackRef);
    if (!self) {
        return;
    }
    self->OnMm2sIoc();
}

/**
 * @brief ERROR割り込みハンドラエントリ
 *
 * @param[in] ref XAudioFormatter*
 */
void AudioFormatterTx::Mm2sErrorCallback(void *ref) noexcept
{
    auto *af   = static_cast<XAudioFormatter *>(ref);
    auto *self = static_cast<AudioFormatterTx *>(af->MM2SERRCallbackRef);
    if (!self) {
        return;
    }
    self->OnMm2sError();
}

/**
 * @brief TIMEOUT割り込みハンドラエントリ
 *
 * @param[in] ref XAudioFormatter*
 */
void AudioFormatterTx::Mm2sTimeoutCallback(void *ref) noexcept
{
    auto *af   = static_cast<XAudioFormatter *>(ref);
    auto *self = static_cast<AudioFormatterTx *>(af->MM2STOCallbackRef);
    if (!self) {
        return;
    }
    self->OnMm2sTimeout();
}

/**
 * @brief 割り込み発生時の処理
 *
 * - IOC回数をインクリメント
 * - PERIODS_PER_CH=16を前提に、半分（8）および終端（16）でフラグを更新する
 *
 * フラグ:
 * - 1: PING 側が空いた（前半の供給が可能）
 * - 2: PONG 側が空いた（後半の供給が可能）
 */
void AudioFormatterTx::OnMm2sIoc()
{
    ioc_count_++;
    uint32_t per = ioc_count_ % (kPeriodsPerCh * 2);

    if (per == kPeriodsPerCh) {
        ready_buffer_side_ = BufferSide::Ping;
    } else if (per == 0) {
        ready_buffer_side_ = BufferSide::Pong;
    }
}

/**
 * @brief ERROR発生時の処理
 */
void AudioFormatterTx::OnMm2sError()
{
    // 一旦ログ出しだけ
    LOGE("DMA Error");
}

/**
 * @brief TIMEOUT発生時の処理
 */
void AudioFormatterTx::OnMm2sTimeout()
{
    // 一旦ログ出しだけ
    LOGE("DMA TimeOut");
}

/**
 * @brief Audio Formatter TXを初期化する
 *
 * @param[in] cfg 初期化設定
 * @retval true  成功
 * @retval false 失敗
 */
bool AudioFormatterTx::Init(const Config &cfg)
{
    LOG_SCOPE();

    if (run_) {
        return true;
    }

    if ((cfg.irq_id == 0u) || (cfg.dma_buf_base == 0u) || (cfg.active_ch == 0u) || (cfg.periods == 0u) ||
        (cfg.bytes_per_period == 0u) || (cfg.bits_per_sample > BIT_DEPTH_32)) {
        LOGE("AudioFormatterTx::Init invalid config");
        return false;
    }

    cfg_ = cfg;

    auto &inst = AudioFormatterCore::GetInstance();
    af_mm2s_   = inst.GetCore();
    if (!af_mm2s_) {
        LOGE("AudioFormatterCore is not initialized");
        return false;
    }

    const uintptr_t reg_base = inst.GetBaseAddr();
    if (reg_base == 0u) {
        LOGE("AudioFormatterCore base addr is invalid");
        return false;
    }

    regs_.SetBase(reg_base);
    XAudioFormatterHwParams af_hw_params = {
        cfg_.dma_buf_base, cfg_.active_ch, cfg_.bits_per_sample, cfg_.periods, cfg_.bytes_per_period,
    };

    if (af_mm2s_->mm2s_presence) {
        af_mm2s_->ChannelId = XAudioFormatter_MM2S;

        {
            auto   &gic    = platform::GicCore::GetInstance();
            XStatus status = gic.Attach(cfg_.irq_id, (Xil_InterruptHandler)XAudioFormatterMM2SIntrHandler, af_mm2s_,
                                        platform::GicCore::GicPriority::Normal, platform::GicCore::GicTrigger::Rising);

            if (status != XST_SUCCESS) {
                LOGE("Audio Formatter MM2S IRQ Attach Failed");
                return false;
            }

            gic.Enable(cfg_.irq_id);
        }

        XAudioFormatter_SetMM2SCallback(af_mm2s_, XAudioFormatter_IOC_Handler,
                                        reinterpret_cast<void *>(&Mm2sIocCallback), this);
        XAudioFormatter_SetMM2SCallback(af_mm2s_, XAudioFormatter_ERROR_Handler,
                                        reinterpret_cast<void *>(&Mm2sErrorCallback), this);
        XAudioFormatter_SetMM2SCallback(af_mm2s_, XAudioFormatter_TIMEOUT_Handler,
                                        reinterpret_cast<void *>(&Mm2sTimeoutCallback), this);

        // 割り込みの有効化
        XAudioFormatter_InterruptEnable(af_mm2s_,
                                        XAUD_CTRL_IOC_IRQ_MASK | XAUD_CTRL_ERR_IRQ_MASK | XAUD_CTRL_TIMEOUT_IRQ_MASK);

        regs_.MULTIPLIER_REG().Write((AF_MCLK / AF_FS));

        {
            // とりあえず以下の設定のみ行う
            uint8_t cs_bytes[24] = {
                0x41,  // Byte0: Professional=1, PCM, Fs=48kHz (bits6-7=01)
                0x01,  // Byte1: Mode=2ch
                0x08,  // Byte2: Word length = 16bit (実語長=16, 最大語長=20)
                0x00,  // Byte3: Channel number (not used)
                0x00,  // Byte4: Ext Fs (not used for 48kHz)
                0x00,  // Byte5
                0x00,  // Byte6
                0x00,  // Byte7
                0x00,  // Byte8
                0x00,  // Byte9
                0x00,  // Byte10
                0x00,  // Byte11
                0x00,  // Byte12
                0x00,  // Byte13
                0x00,  // Byte14
                0x00,  // Byte15
                0x00,  // Byte16
                0x00,  // Byte17
                0x00,  // Byte18
                0x00,  // Byte19
                0x00,  // Byte20
                0x00,  // Byte21
                0x00,  // Byte22
                0x00   // Byte23 (CRC未使用なら0)
            };

            uint32_t cs_word[6] = {0};
            for (uint32_t i = 0; i < 6; i++) {
                cs_word[i] = cs_bytes[i * 4] | (cs_bytes[i * 4 + 1] << 8) | (cs_bytes[i * 4 + 2] << 16) |
                             (cs_bytes[i * 4 + 3] << 24);
            }

            regs_.CH_STATUS_BITS0_REG().Write(cs_word[0]);
            regs_.CH_STATUS_BITS1_REG().Write(cs_word[1]);
            regs_.CH_STATUS_BITS2_REG().Write(cs_word[2]);
            regs_.CH_STATUS_BITS3_REG().Write(cs_word[3]);
            regs_.CH_STATUS_BITS4_REG().Write(cs_word[4]);
            regs_.CH_STATUS_BITS5_REG().Write(cs_word[5]);
        }

        XAudioFormatterSetHwParams(af_mm2s_, &af_hw_params);
        SetBufferBase(cfg_.dma_buf_base);

    } else {
        LOGE("Audio Formatter MM2S Failed");
        return false;
    }

    run_ = true;

    return true;
}

/**
 * @brief Audio Formatter TXを終了する
 */
void AudioFormatterTx::Deinit()
{
    if (!run_) {
        return;
    }

    auto &gic = platform::GicCore::GetInstance();
    gic.Disable(cfg_.irq_id);
    gic.Detach(cfg_.irq_id);

    run_ = false;
}

/**
 * @brief Audio Formatter TXの動作を開始する
 */
void AudioFormatterTx::Start()
{
    if (!run_) {
        return;
    }
    ready_buffer_side_ = BufferSide::None;
    ioc_count_         = 0;
    regs_.CONTROL_REG().SetBits(kRun);
}

/**
 * @brief Audio Formatter TXの動作を停止する
 */
void AudioFormatterTx::Stop()
{
    if (!run_) {
        return;
    }
    regs_.CONTROL_REG().ClrBits(kRun);
}

/**
 * @brief DMAバッファベースアドレスを設定する
 *
 * @param[in] base DMA バッファベース物理アドレス
 */
void AudioFormatterTx::SetBufferBase(uintptr_t base)
{
    regs_.BUFFER_ADDR_LSB_REG().Write(static_cast<uint32_t>(base));
    regs_.BUFFER_ADDR_MSB_REG().Write(static_cast<uint32_t>(static_cast<uint64_t>((uint64_t)base >> 32)));
}

/**
 * @brief ready_buffer_side_を取り出してクリアする
 *
 * @param[out] out 取り出した値
 * @retval true  取り出し成功
 * @retval false フラグ無し、または out==nullptr
 */
bool AudioFormatterTx::ConsumeReadyBufferSide(BufferSide *out)
{
    if (!out) {
        return false;
    }

    InterruptGuard guard;

    BufferSide v = ready_buffer_side_;
    if (v == BufferSide::None) {
        return false;
    }

    ready_buffer_side_ = BufferSide::None;
    *out               = v;

    return true;
}

/**
 * @brief STATUSをクリアし、IOCを有効化
 */
void AudioFormatterTx::EnableIoc()
{
    // ステータスクリア
    uint32_t st = regs_.STATUS_REG().Read();
    if (st) {
        regs_.STATUS_REG().Write(st);
    }

    // IOC有効
    regs_.CONTROL_REG().SetBits(kIocIrqEn);
}

/**
 * @brief チャンク設定を行う
 *
 * @param[in] periods 1チャンク中の period 数
 * @param[in] bpp     1 period あたりのバイト数（Bytes Per Period）
 */
void AudioFormatterTx::SetChunk(uint32_t periods, uint32_t bpp)
{
    uint32_t val = (periods << 16) | bpp;
    regs_.PERIODS_CONFIG_REG().Write(val);

    regs_.CH_OFFSET_REG().Write(bpp / 2U);
}

/**
 * @brief Audio Formatter TXをリセットする
 */
void AudioFormatterTx::Reset()
{
    regs_.CONTROL_REG().ClrBits(kRun);

    // ステータスクリア
    uint32_t st = regs_.STATUS_REG().Read();
    if (st) {
        regs_.STATUS_REG().Write(st);
    }

    ready_buffer_side_ = BufferSide::None;
    ioc_count_         = 0;
}

}  // namespace module
}  // namespace core0
