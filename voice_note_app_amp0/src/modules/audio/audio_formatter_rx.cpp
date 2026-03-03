/**
 * @file audio_formatter_rx.cpp
 * @brief AudioFormatterRxの実装
 */

// 自ヘッダー
#include "audio_formatter_rx.h"

// Vitisライブラリ
#include "sleep.h"
#include "xstatus.h"

// プロジェクトライブラリ
#include "audio_format.h"
#include "logger_core.h"
#include "utility.h"

namespace core0 {
namespace module {

/**
 * @brief コールバック
 *
 * @param[in] ref XAudioFormatter*
 */
void AudioFormatterRx::S2mmIocCallback(void *ref) noexcept
{
    auto *af   = static_cast<XAudioFormatter *>(ref);
    auto *self = static_cast<AudioFormatterRx *>(af->S2MMIOCCallbackRef);
    if (!self) {
        return;
    }
    self->OnS2mmIoc();
}

/**
 * @brief 割り込み発生時の処理
 *
 * - IOC回数をインクリメント
 * - PERIODS_PER_CH=16 を前提に、半分（8）および終端（16）でフラグを更新する
 *
 * フラグ:
 * - 1: PING 側が空いた（前半の供給が可能）
 * - 2: PONG 側が空いた（後半の供給が可能）
 */
void AudioFormatterRx::OnS2mmIoc()
{
    ioc_count_++;
    uint32_t per = ioc_count_ % (kPeriodsPerHalf * 2);

    if (per == kPeriodsPerHalf) {
        ready_buffer_side_ = BufferSide::Ping;
    } else if (per == 0) {
        ready_buffer_side_ = BufferSide::Pong;
    }
}

/**
 * @brief Audio Formatter RXを初期化する
 *
 * @param[in] cfg 初期化設定
 * @retval true  成功
 * @retval false 失敗
 */
bool AudioFormatterRx::Init(const Config &cfg)
{
    LOG_SCOPE();

    if (run_) {
        return true;
    }

    if ((cfg.irq_id == 0u) || (cfg.dma_buf_base == 0u) || (cfg.active_ch == 0u) || (cfg.periods == 0u) ||
        (cfg.bytes_per_period == 0u) || (cfg.bits_per_sample > BIT_DEPTH_32)) {
        LOGE("AudioFormatterRx::Init invalid config");
        return false;
    }

    cfg_ = cfg;

    auto &inst = AudioFormatterCore::GetInstance();
    af_s2mm_   = inst.GetCore();
    if (!af_s2mm_) {
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

    if (af_s2mm_->s2mm_presence) {
        af_s2mm_->ChannelId = XAudioFormatter_S2MM;

        {
            auto   &gic    = platform::GicCore::GetInstance();
            XStatus status = gic.Attach(cfg_.irq_id, (Xil_InterruptHandler)XAudioFormatterS2MMIntrHandler, af_s2mm_,
                                        platform::GicCore::GicPriority::Normal, platform::GicCore::GicTrigger::Rising);

            if (status != XST_SUCCESS) {
                LOGE("Audio Formatter S2MM IRQ Attach Failed");
                return false;
            }

            gic.Enable(cfg_.irq_id);
        }

        XAudioFormatter_SetS2MMCallback(af_s2mm_, XAudioFormatter_IOC_Handler,
                                        reinterpret_cast<void *>(&S2mmIocCallback), this);

        // 割り込みの有効化
        XAudioFormatter_InterruptEnable(af_s2mm_, XAUD_CTRL_IOC_IRQ_MASK);

        regs_.TIMEOUT_REG().Write(AF_S2MM_TIMEOUT);

        XAudioFormatterSetHwParams(af_s2mm_, &af_hw_params);
        SetBufferBase(cfg_.dma_buf_base);
    } else {
        LOGE("Audio Formatter S2MM Failed");
        return false;
    }

    run_ = true;

    return true;
}

/**
 * @brief Audio Formatter RXを終了する
 */
void AudioFormatterRx::Deinit()
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
 * @brief Audio Formatter RXの動作を開始する
 */
void AudioFormatterRx::Start()
{
    if (!run_) {
        return;
    }
    ready_buffer_side_ = BufferSide::None;
    ioc_count_         = 0;
    regs_.CONTROL_REG().SetBits(kRun);
}

/**
 * @brief Audio Formatter RXの動作を停止する
 */
void AudioFormatterRx::Stop()
{
    if (!run_) {
        return;
    }
    regs_.CONTROL_REG().ClrBits(kRun);
}

/**
 * @brief DMAバッファのベースアドレスを設定する
 *
 * @param[in] base DMA バッファベース物理アドレス
 */
void AudioFormatterRx::SetBufferBase(uintptr_t base)
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
bool AudioFormatterRx::ConsumeReadyBufferSide(BufferSide *out)
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
void AudioFormatterRx::EnableIoc()
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
 * @param[in] periods 1チャンク中のperiod数
 * @param[in] bpp     1periodあたりのバイト数
 */
void AudioFormatterRx::SetChunk(uint32_t periods, uint32_t bpp)
{
    uint32_t val = (periods << 16) | bpp;
    regs_.PERIODS_CONFIG_REG().Write(val);

    regs_.CH_OFFSET_REG().Write(bpp / 2U);
}

/**
 * @brief Audio Formatter RXをリセットする
 */
void AudioFormatterRx::Reset()
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
