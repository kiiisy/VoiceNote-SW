/**
 * @file agc_core.cpp
 * @brief Agcの実装
 */

// 自ヘッダー
#include "agc_core.h"

// 標準ライブラリ
#include <algorithm>

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace platform {

uint32_t Agc::ToQ2_14(float g)
{
    // float -> signed Q2.14 -> low16
    constexpr int32_t SCALE = 1 << 14;

    int32_t v = static_cast<int32_t>(g * SCALE + (g >= 0 ? 0.5f : -0.5f));  // 丸め
    v         = std::min<int32_t>(v, 0x7FFF);
    v         = std::max<int32_t>(v, -0x8000);

    return static_cast<uint32_t>(v) & 0xFFFF;
}

void Agc::Init(uintptr_t base_addr)
{
    base_ = base_addr;
    regs_ = Regs(base_addr);
}

void Agc::ApplyDefault()
{
    LOG_SCOPE();

    Config cfg{};
    ApplyConfig(cfg);
}

void Agc::ApplyConfig(const Config &cfg)
{
    LOG_SCOPE();

    // CONTROL bit0 MANUAL_MODE, bit1 RESET_IIR, bit2 FREEZE_GAIN
    uint32_t ctrl = 0;
    if (cfg.manual_mode) {
        ctrl |= (1u << 0);
    }
    if (cfg.reset_iir) {
        ctrl |= (1u << 1);
    }
    if (cfg.freeze_gain) {
        ctrl |= (1u << 2);
    }

    regs_.CONTROL().Write(ctrl);

    regs_.DIST_SENS_MM().Write(cfg.dist_sens_mm);
    regs_.ALPHA_CFG().Write(cfg.alpha_shift & 0xF);

    regs_.GAIN_MIN().Write(ToQ2_14(cfg.gain_min));
    regs_.GAIN_MAX().Write(ToQ2_14(cfg.gain_max));
}

}  // namespace platform
}  // namespace core0
