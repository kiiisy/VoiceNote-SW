/**
 * @file acu_core.h
 * @brief Audio Clean Up(ACU) IP制御クラス定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "reg_core.h"

namespace core0 {
namespace platform {

/**
 * @class Acu
 * @brief Audio Clean Up IP制御クラス
 */
class Acu final
{
public:
    static Acu &GetInstance()
    {
        static Acu inst;
        return inst;
    }

    struct DcCutConfig
    {
        float    fs_hz   = 48000.0f;
        float    fc_hz   = 20.0f;  // cutoff
        uint32_t dc_pass = 0x1;    // 0: enable, 1: bypass
    };

    struct NoiseGateConfig
    {
        float    th_open   = 0.05f;
        float    th_close  = 0.04f;
        float    attack_s  = 0.005f;
        float    release_s = 0.050f;
        uint32_t ng_pass   = 0x1;  // 0: enable, 1: bypass
    };

    void Init(uintptr_t base_addr);

    void ApplyDefault();

    // 個別設定
    void SetDcCut(const DcCutConfig &cfg);
    void SetNoiseGate(const NoiseGateConfig &cfg);

    void Start(bool auto_restart = true);

private:
    Acu() = default;
    ~Acu() {}

    struct Regs
    {
        //------------------------Address Info-------------------
        // Protocol Used: ap_ctrl_hs
        //
        // 0x00 : Control signals
        //        bit 0  - ap_start (Read/Write/COH)
        //        bit 1  - ap_done (Read/COR)
        //        bit 2  - ap_idle (Read)
        //        bit 3  - ap_ready (Read/COR)
        //        bit 7  - auto_restart (Read/Write)
        //        bit 9  - interrupt (Read)
        //        others - reserved
        // 0x04 : Global Interrupt Enable Register
        //        bit 0  - Global Interrupt Enable (Read/Write)
        //        others - reserved
        // 0x08 : IP Interrupt Enable Register (Read/Write)
        //        bit 0 - enable ap_done interrupt (Read/Write)
        //        bit 1 - enable ap_ready interrupt (Read/Write)
        //        others - reserved
        // 0x0c : IP Interrupt Status Register (Read/TOW)
        //        bit 0 - ap_done (Read/TOW)
        //        bit 1 - ap_ready (Read/TOW)
        //        others - reserved
        // 0x10 : Data signal of dc_a_coef
        //        bit 31~0 - dc_a_coef[31:0] (Read/Write)
        // 0x14 : reserved
        // 0x18 : Data signal of dc_pass
        //        bit 0  - dc_pass[0] (Read/Write)
        //        others - reserved
        // 0x1c : reserved
        // 0x20 : Data signal of th_open_amp
        //        bit 31~0 - th_open_amp[31:0] (Read/Write)
        // 0x24 : Data signal of th_open_amp
        //        bit 1~0 - th_open_amp[33:32] (Read/Write)
        //        others  - reserved
        // 0x28 : reserved
        // 0x2c : Data signal of th_close_amp
        //        bit 31~0 - th_close_amp[31:0] (Read/Write)
        // 0x30 : Data signal of th_close_amp
        //        bit 1~0 - th_close_amp[33:32] (Read/Write)
        //        others  - reserved
        // 0x34 : reserved
        // 0x38 : Data signal of a_attack
        //        bit 23~0 - a_attack[23:0] (Read/Write)
        //        others   - reserved
        // 0x3c : reserved
        // 0x40 : Data signal of a_release
        //        bit 23~0 - a_release[23:0] (Read/Write)
        //        others   - reserved
        // 0x44 : reserved
        // 0x48 : Data signal of b_attack
        //        bit 23~0 - b_attack[23:0] (Read/Write)
        //        others   - reserved
        // 0x4c : reserved
        // 0x50 : Data signal of b_release
        //        bit 23~0 - b_release[23:0] (Read/Write)
        //        others   - reserved
        // 0x54 : reserved
        // 0x58 : Data signal of ng_pass
        //        bit 0  - ng_pass[0] (Read/Write)
        //        others - reserved
        // 0x5c : reserved
        // (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)
        explicit Regs(uintptr_t base) : blk_(base) {}

        // ap_ctrl_hs
        inline Reg32 CTRL() const { return blk_.Reg(0x00); }

        // DC-cut
        inline Reg32 DC_A_COEF() const { return blk_.Reg(0x10); }
        inline Reg32 DC_PASS() const { return blk_.Reg(0x18); }

        // NoiseGate threshold (Q34 as 64bit split)
        inline Reg32 TH_OPEN_L() const { return blk_.Reg(0x20); }
        inline Reg32 TH_OPEN_H() const { return blk_.Reg(0x24); }
        inline Reg32 TH_CLOSE_L() const { return blk_.Reg(0x2C); }
        inline Reg32 TH_CLOSE_H() const { return blk_.Reg(0x30); }

        // Attack/Release (raw)
        inline Reg32 A_ATTACK() const { return blk_.Reg(0x38); }
        inline Reg32 A_RELEASE() const { return blk_.Reg(0x40); }
        inline Reg32 B_ATTACK() const { return blk_.Reg(0x48); }
        inline Reg32 B_RELEASE() const { return blk_.Reg(0x50); }

        // NG bypass
        inline Reg32 NG_PASS() const { return blk_.Reg(0x58); }

    private:
        RegCore blk_;
    };

    uintptr_t base_{0};
    Regs      regs_{0};
};

}  // namespace platform
}  // namespace core0
