/**
 * @file arec_core.cpp
 * @brief Arecの実装
 */

// 自ヘッダー
#include "arec_core.h"

// 標準ライブラリ
#include <algorithm>

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace platform {
namespace {

uint32_t ClampField(uint32_t value, uint32_t mask) { return std::min(value, mask); }

}  // namespace

void Arec::IrqHandler(void *ref) { static_cast<Arec *>(ref)->OnIrq(); }

void Arec::OnIrq()
{
    irq_pending_ = true;
    ClearIrq();
}

XStatus Arec::Init(uintptr_t base_addr, uint32_t irq_id)
{
    LOG_SCOPE();

    if (run_) {
        LOGI("Arec Already Started");
        return XST_SUCCESS;
    }

    if (base_addr == 0u || irq_id == 0u) {
        LOGE("Arec invalid config");
        return XST_FAILURE;
    }

    base_   = base_addr;
    regs_   = Regs(base_addr);
    irq_id_ = irq_id;

    auto   &gic    = GicCore::GetInstance();
    XStatus status = gic.Attach(irq_id_, (Xil_InterruptHandler)&Arec::IrqHandler, this, GicCore::GicPriority::Normal,
                                GicCore::GicTrigger::Rising);
    if (status != XST_SUCCESS) {
        LOGE("Arec IRQ Attach Failed");
        return status;
    }

    gic.Enable(irq_id_);

    run_ = true;

    return XST_SUCCESS;
}

void Arec::Deinit()
{
    if (!run_) {
        return;
    }

    Disable();

    auto &gic = GicCore::GetInstance();
    gic.Disable(irq_id_);
    gic.Detach(irq_id_);

    run_    = false;
    irq_id_ = 0;
    auto_record_mode_enabled_ = false;
    irq_pending_              = false;
}

void Arec::ApplyDefault()
{
    LOG_SCOPE();

    Config cfg{};
    ApplyConfig(cfg);
}

void Arec::ApplyConfig(const Config &cfg)
{
    LOG_SCOPE();

    auto_record_mode_enabled_ = cfg.enable;
    irq_pending_              = false;

    // 動作中書き換えを避けるため、いったん停止してから各パラメータを反映する。
    Disable();

    regs_.THRESHOLD().Write(ClampField(cfg.threshold, kThresholdMask));
    regs_.WINDOW_SAMPLES().Write(ClampField(cfg.window_shift, kWindowSamplesMask));
    regs_.PRETRIG_SAMPLES().Write(ClampField(cfg.pretrig_samples, kPretrigSamplesMask));
    regs_.REQUIRED_WINDOWS().Write(ClampField(cfg.required_windows, kRequiredWindowsMask));
}

void Arec::Enable()
{
    if (!run_) {
        LOGW("Arec Not Started");
        return;
    }

    regs_.CONTROL().Write(kControlEnableMask);
}

void Arec::Disable()
{
    if (!run_) {
        return;
    }

    regs_.CONTROL().Write(0u);
}

void Arec::ClearIrq()
{
    if (!run_) {
        return;
    }

    const uint32_t keep_enable = regs_.CONTROL().Read() & kControlEnableMask;
    regs_.CONTROL().Write(keep_enable | kControlIrqClearMask);
}

Arec::Status Arec::GetStatus() const
{
    const uint32_t raw = regs_.STATUS_STATE().Read();

    Status status{};
    status.state             = static_cast<Status::State>((raw & kStatusStateMask) >> kStatusStateShift);
    status.pretrig_ready     = (raw & kStatusPretrigReadyMask) != 0u;
    status.en_rd             = (raw & kStatusEnRdMask) != 0u;
    status.en_wr             = (raw & kStatusEnWrMask) != 0u;
    status.is_dump           = (raw & kStatusIsDumpMask) != 0u;
    status.dump_done         = (raw & kStatusDumpDoneMask) != 0u;
    status.triggered_latched = (raw & kStatusTriggeredMask) != 0u;

    return status;
}

}  // namespace platform
}  // namespace core0
