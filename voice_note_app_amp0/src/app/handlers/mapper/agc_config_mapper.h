/**
 * @file agc_config_mapper.h
 * @brief IPCのAGC設定を内部AGC設定へ変換する
 */
#pragma once

// 標準ライブラリ
#include <algorithm>
#include <cstdint>

// プロジェクトライブラリ
#include "agc_core.h"
#include "ipc_msg.h"

namespace core0 {
namespace app {

/**
 * @brief IPCのAGC設定を内部設定へ変換する
 *
 * @param[in] p IPCのAGC設定
 * @return 変換後のAGC設定
 */
inline core0::platform::Agc::Config ToAgcConfig(const core::ipc::SetAgcPayload &p)
{
    using core0::platform::Agc;

    Agc::Config cfg{};

    // speed_k -> alpha_shift (0..15想定)
    const int32_t k = std::clamp<int32_t>(p.speed_k, 0, 15);
    cfg.alpha_shift = static_cast<uint32_t>(k);

    // dist_mm -> dist_sens_mm
    const int32_t dist = std::clamp<int32_t>(p.dist_mm, 0, 30000);
    cfg.dist_sens_mm   = static_cast<uint32_t>(dist);

    // x100 -> float
    auto x100_to_gain = [](int16_t v) -> float {
        const int32_t vv = std::clamp<int32_t>(v, 0, 1000);
        return static_cast<float>(vv) / 100.0f;
    };

    float gmin = x100_to_gain(p.min_gain_x100);
    float gmax = x100_to_gain(p.max_gain_x100);
    if (gmin > gmax) {
        std::swap(gmin, gmax);
    }

    // enable=0 は manual_mode で固定ゲイン1.0 にする
    if (p.enable == 0) {
        cfg.manual_mode = true;
        cfg.gain_min = 1.0f;
        cfg.gain_max = 1.0f;
    } else {
        cfg.manual_mode = false;
        cfg.gain_min = gmin;
        cfg.gain_max = gmax;
    }

    // 更新時の挙動
    cfg.reset_iir   = false;
    cfg.freeze_gain = false;

    return cfg;
}

}  // namespace app
}  // namespace core0
