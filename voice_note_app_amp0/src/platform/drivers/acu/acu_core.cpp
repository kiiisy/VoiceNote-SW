/**
 * @file acu_core.cpp
 * @brief Audio Clean Up(ACU) IP 制御クラス実装
 */

// 自ヘッダー
#include "acu_core.h"

// 標準ライブラリ
#include <cmath>
#include <cstring>

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace platform {

uint32_t FloatBits(float x)
{
    uint32_t u = 0;
    static_assert(sizeof(float) == sizeof(uint32_t), "float must be 32-bit");
    std::memcpy(&u, &x, sizeof(u));

    return u;
}

void Acu::Init(uintptr_t base_addr)
{
    base_ = base_addr;
    regs_ = Regs(base_addr);
}

void Acu::ApplyDefault()
{
    LOG_SCOPE();

    DcCutConfig     dc{};
    NoiseGateConfig ng{};

    SetDcCut(dc);
    SetNoiseGate(ng);

    Start(true);
}

void Acu::SetDcCut(const DcCutConfig &cfg)
{
    LOG_SCOPE();

    // coef = exp(-2*pi*fc/fs)
    const float coef = std::exp(-2.0f * static_cast<float>(M_PI) * cfg.fc_hz / cfg.fs_hz);
    regs_.DC_A_COEF().Write(FloatBits(coef));
    regs_.DC_PASS().Write(cfg.dc_pass);
}

void Acu::SetNoiseGate(const NoiseGateConfig &cfg)
{
    LOG_SCOPE();

    // th_open/th_close: float -> Q15 -> sign extend to 64bit
    auto to_q15 = [](float v) -> int32_t { return static_cast<int32_t>(std::round(v * 32768.0f)); };

    const int32_t th_open_q15  = to_q15(cfg.th_open);
    const int32_t th_close_q15 = to_q15(cfg.th_close);

    const int64_t th_open_q34  = static_cast<int64_t>(th_open_q15);
    const int64_t th_close_q34 = static_cast<int64_t>(th_close_q15);

    regs_.TH_OPEN_L().Write(static_cast<uint32_t>(th_open_q34));
    regs_.TH_OPEN_H().Write(static_cast<uint32_t>(static_cast<uint64_t>(th_open_q34) >> 32));
    regs_.TH_CLOSE_L().Write(static_cast<uint32_t>(th_close_q34));
    regs_.TH_CLOSE_H().Write(static_cast<uint32_t>(static_cast<uint64_t>(th_close_q34) >> 32));

    // Attack/Release IIR係数
    // A = exp(-1/(fs*T))
    // raw = A * 2^22
    // b = 2^22 - a
    // ※ fs は 48kHz 前提
    constexpr float fs = 48000.0f;

    const float A = std::exp(-1.0f / (fs * cfg.attack_s));
    const float R = std::exp(-1.0f / (fs * cfg.release_s));

    const int32_t a_attack_raw  = static_cast<int32_t>(A * (1 << 22));
    const int32_t a_release_raw = static_cast<int32_t>(R * (1 << 22));
    const int32_t b_attack_raw  = (1 << 22) - a_attack_raw;
    const int32_t b_release_raw = (1 << 22) - a_release_raw;

    regs_.A_ATTACK().Write(static_cast<uint32_t>(a_attack_raw));
    regs_.A_RELEASE().Write(static_cast<uint32_t>(a_release_raw));
    regs_.B_ATTACK().Write(static_cast<uint32_t>(b_attack_raw));
    regs_.B_RELEASE().Write(static_cast<uint32_t>(b_release_raw));

    regs_.NG_PASS().Write(cfg.ng_pass);
}

void Acu::Start(bool auto_restart)
{
    LOG_SCOPE();

    uint32_t v = 0x01;

    if (auto_restart) {
        v |= (1u << 7);
    }

    regs_.CTRL().Write(v);
}

}  // namespace platform
}  // namespace core0
