/**
 * @file agc_core.h
 * @brief AGC IP 制御クラス定義（ToF距離→ゲイン）
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "reg_core.h"

namespace core0 {
namespace platform {

/**
 * @class Agc
 * @brief AGC IP 制御クラス
 */
class Agc final
{
public:
    static Agc &GetInstance()
    {
        static Agc inst;
        return inst;
    }

    struct Config
    {
        uint32_t dist_sens_mm = 500;    // これ以上距離が動いたら更新
        uint32_t alpha_shift  = 6;      // IIR: 1/2^alpha_shift
        float    gain_min     = 0.5f;   // x0.5
        float    gain_max     = 2.0f;   // x2.0
        bool     reset_iir    = false;  // CONTROL bit1
        bool     freeze_gain  = false;  // CONTROL bit2
    };

    void Init(uintptr_t base_addr);

    // 現状の SetAGC と同等のデフォルト設定
    void ApplyDefault();

    void ApplyConfig(const Config &cfg);

    // ステータスダンプ（元 DumpAGCStatus）
    void DumpStatus() const;

private:
    Agc() = default;
    ~Agc() {}

    struct Regs
    {
        explicit Regs(uintptr_t base) : blk_(base) {}

        inline Reg32 CONTROL() const { return blk_.Reg(0x00); }

        // status/readbacks
        inline Reg32 STATUS() const { return blk_.Reg(0x04); }
        inline Reg32 DIST_RAW_MM() const { return blk_.Reg(0x08); }
        inline Reg32 DIST_CLAMP_MM() const { return blk_.Reg(0x0C); }
        inline Reg32 DIST_SENS_MM() const { return blk_.Reg(0x10); }
        inline Reg32 GAIN_TARGET() const { return blk_.Reg(0x18); }  // Q2.14
        inline Reg32 GAIN_SMOOTH() const { return blk_.Reg(0x1C); }  // Q2.14

        // configs
        inline Reg32 GAIN_MIN() const { return blk_.Reg(0x20); }   // Q2.14 (low 16)
        inline Reg32 GAIN_MAX() const { return blk_.Reg(0x24); }   // Q2.14 (low 16)
        inline Reg32 ALPHA_CFG() const { return blk_.Reg(0x28); }  // low 4

    private:
        RegCore blk_;
    };

    static uint32_t ToQ2_14(float g);

    uintptr_t base_ = 0;
    Regs      regs_{0};
};
}  // namespace platform
}  // namespace core0
