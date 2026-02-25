/**
 * @file audio_formatter_rx.h
 * @brief Audio Formatter RX（録音側）制御クラス
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_formatter_core.h"
#include "common.h"
#include "constant.h"
#include "gic_core.h"
#include "reg_core.h"

namespace core0 {
namespace module {

class AudioBppPool;

/**
 * @class AudioFormatterRx
 * @brief Audio Formatter RX制御クラス
 */
class AudioFormatterRx final
{
public:
    struct Config
    {
        uint32_t  irq_id{};
        uintptr_t dma_buf_base{};
        uint32_t  active_ch{};
        uint32_t  bits_per_sample{};
        uint32_t  periods{};
        uint32_t  bytes_per_period{};
    };

    static AudioFormatterRx &GetInstance()
    {
        static AudioFormatterRx inst;
        return inst;
    }

    AudioFormatterRx(const AudioFormatterRx &)            = delete;
    AudioFormatterRx &operator=(const AudioFormatterRx &) = delete;
    AudioFormatterRx(AudioFormatterRx &&)                 = delete;
    AudioFormatterRx &operator=(AudioFormatterRx &&)      = delete;
    AudioFormatterRx()                                    = default;

    enum class BufferSide : uint8_t
    {
        None = 0,
        Ping = 1,
        Pong = 2
    };

    bool Init(const Config &cfg);
    void Deinit();
    void Start();
    void Stop();
    void SetBufferBase(uintptr_t base);
    bool ConsumeReadyBufferSide(BufferSide *out);
    void EnableIoc();
    void SetChunk(uint32_t periods, uint32_t bpp);
    void Reset();

    void      ClearIocCount() { ioc_count_ = 0; };
    uint32_t  GetIocCount() { return ioc_count_; };
    uintptr_t GetBufBase() { return cfg_.dma_buf_base; };

private:
    /**
     * @struct Regs
     * @brief Audio Formatter RXレジスタアクセスラッパー
     */
    struct Regs
    {
        explicit Regs(uintptr_t base) : blk_(base) {}
        inline void SetBase(uintptr_t base) { blk_ = platform::RegCore(base); }

        inline platform::Reg32 CONTROL_REG() const { return blk_.Reg(s2mm::CONTROL); }
        inline platform::Reg32 STATUS_REG() const { return blk_.Reg(s2mm::STATUS); }
        inline platform::Reg32 TIMEOUT_REG() const { return blk_.Reg(s2mm::TIMEOUT); }
        inline platform::Reg32 PERIODS_CONFIG_REG() const { return blk_.Reg(s2mm::PERIODS_CONFIG); }
        inline platform::Reg32 BUFFER_ADDR_LSB_REG() const { return blk_.Reg(s2mm::BUFFER_ADDR_LSB); }
        inline platform::Reg32 BUFFER_ADDR_MSB_REG() const { return blk_.Reg(s2mm::BUFFER_ADDR_MSB); }
        inline platform::Reg32 CH_STATUS_BITS0_REG() const { return blk_.Reg(s2mm::CH_STATUS_BITS0); }
        inline platform::Reg32 CH_STATUS_BITS1_REG() const { return blk_.Reg(s2mm::CH_STATUS_BITS1); }
        inline platform::Reg32 CH_STATUS_BITS2_REG() const { return blk_.Reg(s2mm::CH_STATUS_BITS2); }
        inline platform::Reg32 CH_STATUS_BITS3_REG() const { return blk_.Reg(s2mm::CH_STATUS_BITS3); }
        inline platform::Reg32 CH_STATUS_BITS5_REG() const { return blk_.Reg(s2mm::CH_STATUS_BITS5); }
        inline platform::Reg32 CH_OFFSET_REG() const { return blk_.Reg(s2mm::CH_OFFSET); }

    private:
        platform::RegCore blk_;
    };

    static void S2mmIocCallback(void *ref) noexcept;
    void        OnS2mmIoc();

    Regs   regs_{0};  // S2MMレジスタブロック（InitでSetBase）
    Config cfg_{};

    XAudioFormatter *af_s2mm_{nullptr};  // Audio Formatterドライバハンドル

    bool run_{false};  // 初期化済み/稼働可能フラグ

    volatile uint32_t   ioc_count_{0};  // 割り込み回数
    volatile BufferSide ready_buffer_side_{BufferSide::None};
};

}  // namespace module
}  // namespace core0
