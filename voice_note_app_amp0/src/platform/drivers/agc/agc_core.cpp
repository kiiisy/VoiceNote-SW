/**
 * @file agc_core.cpp
 * @brief AGC IP 制御クラス実装
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

    // CONTROL bit1 RESET_IIR, bit2 FREEZE_GAIN
    uint32_t ctrl = 0;
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

void Agc::DumpStatus() const
{

    const uint32_t status        = regs_.STATUS().Read();
    const uint32_t dist_raw_mm   = regs_.DIST_RAW_MM().Read();
    const uint32_t dist_clamp_mm = regs_.DIST_CLAMP_MM().Read();
    const uint32_t gain_target   = regs_.GAIN_TARGET().Read();  // Q2.14 (low16 signed)
    const uint32_t gain_smooth   = regs_.GAIN_SMOOTH().Read();  // Q2.14 (low16 signed)

    const bool tof_working = (status & (1u << 0)) != 0;
    const bool frame_error = (status & (1u << 1)) != 0;
    const bool pkt_error   = (status & (1u << 2)) != 0;
    const bool clip_flg    = (status & (1u << 4)) != 0;

    auto q14_to_float = [](int16_t q) -> float { return static_cast<float>(q) / static_cast<float>(1 << 14); };

    LOGI("AGC status:");
    LOGI("  tof_working = %d, pkt_error = %d, clip = %d, frame_error = %d", tof_working, pkt_error, clip_flg,
         frame_error);
    LOGI("  dist_raw   = %lu mm, dist_clamp = %lu mm", static_cast<unsigned long>(dist_raw_mm),
         static_cast<unsigned long>(dist_clamp_mm));
    LOGI("  gain_target = 0x%04lX (%.3f)", static_cast<unsigned long>(gain_target & 0xFFFF),
         q14_to_float(static_cast<int16_t>(gain_target & 0xFFFF)));
    LOGI("  gain_smooth = 0x%04lX (%.3f)", static_cast<unsigned long>(gain_smooth & 0xFFFF),
         q14_to_float(static_cast<int16_t>(gain_smooth & 0xFFFF)));
}

}  // namespace platform
}  // namespace core0
