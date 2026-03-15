/**
 * @file arec_core.h
 * @brief Auto Record(AREC) IP制御クラス定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xstatus.h"

// プロジェクトライブラリ
#include "gic_core.h"
#include "reg_core.h"

namespace core0 {
namespace platform {

/**
 * @class Arec
 * @brief Auto Record IP制御クラス
 */
class Arec final
{
public:
    static Arec &GetInstance()
    {
        static Arec inst;
        return inst;
    }

    /**
     * @brief ARECの設定値
     */
    struct Config
    {
        uint16_t threshold        = 0x0200;
        uint8_t  window_shift     = 6;
        uint16_t pretrig_samples  = 512;
        uint8_t  required_windows = 2;
        bool     enable           = false;
    };

    /**
     * @brief STATUS_STATEのスナップショット
     */
    struct Status
    {
        enum class State : uint8_t
        {
            kPass  = 0,
            kArmed = 1,
            kDump  = 2,
        };

        State state             = State::kPass;
        bool  pretrig_ready     = false;
        bool  en_rd             = false;
        bool  en_wr             = false;
        bool  is_dump           = false;
        bool  dump_done         = false;
        bool  triggered_latched = false;
    };

    XStatus Init(uintptr_t base_addr, uint32_t irq_id);
    void    Deinit();

    void ApplyDefault();
    void ApplyConfig(const Config &cfg);

    void Enable();
    void Disable();
    void ClearIrq();

    Status GetStatus() const;
    bool   IsAutoRecordModeEnabled() const { return auto_record_mode_enabled_; }
    bool   HasPendingIrq() const { return irq_pending_; }
    void   ClearPendingIrq() { irq_pending_ = false; }

private:
    Arec() = default;
    ~Arec() {}

    static void IrqHandler(void *ref);
    void        OnIrq();

    struct Regs
    {
        explicit Regs(uintptr_t base) : blk_(base) {}

        inline Reg32 CONTROL() const { return blk_.Reg(0x00); }
        inline Reg32 STATUS_STATE() const { return blk_.Reg(0x04); }
        inline Reg32 THRESHOLD() const { return blk_.Reg(0x10); }
        inline Reg32 WINDOW_SAMPLES() const { return blk_.Reg(0x14); }
        inline Reg32 PRETRIG_SAMPLES() const { return blk_.Reg(0x18); }
        inline Reg32 REQUIRED_WINDOWS() const { return blk_.Reg(0x1C); }

    private:
        RegCore blk_;
    };

    static constexpr uint32_t kControlEnableMask      = 0x00000001u;
    static constexpr uint32_t kControlIrqClearMask    = 0x00000002u;
    static constexpr uint32_t kThresholdMask          = 0x0000FFFFu;
    static constexpr uint32_t kWindowSamplesMask      = 0x0000001Fu;
    static constexpr uint32_t kPretrigSamplesMask     = 0x00000FFFu;
    static constexpr uint32_t kRequiredWindowsMask    = 0x0000000Fu;
    static constexpr uint32_t kStatusTriggeredMask    = 0x00000001u;
    static constexpr uint32_t kStatusDumpDoneMask     = 0x00000002u;
    static constexpr uint32_t kStatusIsDumpMask       = 0x00000004u;
    static constexpr uint32_t kStatusEnWrMask         = 0x00000008u;
    static constexpr uint32_t kStatusEnRdMask         = 0x00000010u;
    static constexpr uint32_t kStatusPretrigReadyMask = 0x00000020u;
    static constexpr uint32_t kStatusStateShift       = 16u;
    static constexpr uint32_t kStatusStateMask        = 0x00070000u;

    uintptr_t base_{0};
    Regs      regs_{0};
    bool      run_{false};
    uint32_t  irq_id_{0};
    bool      auto_record_mode_enabled_{false};
    volatile bool irq_pending_{false};
};

}  // namespace platform
}  // namespace core0
