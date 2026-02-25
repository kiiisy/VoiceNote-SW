/**
 * @file i2s_tx_core.h
 * @brief I2S Transmitter制御クラス
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xi2stx.h"
#include "xil_io.h"
#include "xparameters.h"
#include "xstatus.h"

// プロジェクトライブラリ
#include "constant.h"
#include "gic_core.h"
#include "reg_core.h"

namespace core0 {
namespace platform {

/**
 * @class I2sTx
 * @brief I2S Transmitter IP制御クラス
 */
class I2sTx final
{
public:
    struct Config
    {
        uintptr_t base_addr;
        uintptr_t mux_base_addr;
        uint32_t  irq_id;
    };

    static I2sTx &GetInstance()
    {
        static I2sTx inst;
        return inst;
    }

    XStatus Init(const Config &cfg);
    void    Deinit();

    // 操作系
    XStatus Enable();
    void    Disable();

    // 状態
    bool     ok() const { return run_; }
    uint32_t blockCount() const { return blk_count_; }
    void     clearBlockCount() { blk_count_ = 0; }
    XI2s_Tx *handle() { return &i2s_; }

    void DumpI2STxRegs();

private:
    I2sTx() = default;
    ~I2sTx() {}

    /**
     * @struct I2sTx::Regs
     * @brief I2S Tx / Clock MUX レジスタアクセス用ラッパー
     */
    struct Regs
    {
        explicit Regs(uintptr_t base) : blk_(base) {}
        inline void SetBase(uintptr_t base) { blk_ = RegCore(base); }

        inline Reg32 CONTROL_REG() const { return blk_.Reg(i2stx::CONTROL); }
        inline Reg32 VALIDITY_REG() const { return blk_.Reg(i2stx::VALIDITY); }
        inline Reg32 IRQ_ENABLE_REG() const { return blk_.Reg(i2stx::IRQ_ENABLE); }
        inline Reg32 IRQ_STATUS_REG() const { return blk_.Reg(i2stx::IRQ_STATUS); }
        inline Reg32 SCLK_DIV_REG() const { return blk_.Reg(i2stx::SCLK_DIV); }
        inline Reg32 CH_CTRL_01_REG() const { return blk_.Reg(i2stx::CH_CTRL_01); }
        inline Reg32 CH_CTRL_23_REG() const { return blk_.Reg(i2stx::CH_CTRL_23); }
        inline Reg32 CH_CTRL_45_REG() const { return blk_.Reg(i2stx::CH_CTRL_45); }
        inline Reg32 CH_CTRL_67_REG() const { return blk_.Reg(i2stx::CH_CTRL_67); }
        inline Reg32 AES_STATUS_0_REG() const { return blk_.Reg(i2stx::AES_STATUS_0); }
        inline Reg32 AES_STATUS_1_REG() const { return blk_.Reg(i2stx::AES_STATUS_1); }
        inline Reg32 AES_STATUS_2_REG() const { return blk_.Reg(i2stx::AES_STATUS_2); }
        inline Reg32 AES_STATUS_3_REG() const { return blk_.Reg(i2stx::AES_STATUS_3); }
        inline Reg32 AES_STATUS_4_REG() const { return blk_.Reg(i2stx::AES_STATUS_4); }
        inline Reg32 AES_STATUS_5_REG() const { return blk_.Reg(i2stx::AES_STATUS_5); }
        inline Reg32 MUX_CONTROL_REG() const { return blk_.Reg(i2stx::MUX_CONTROL); }

    private:
        RegCore blk_;
    };

    static void AesBlockCmplHandlerEntry(void *ref);
    static void AesBlockErrHandlerEntry(void *ref);
    static void AesChStatHandlerEntry(void *ref);
    static void UnderflowHandlerEntry(void *ref);

    void onAesBlockComplete();
    void onAesBlockError();
    void onAesChStatus();
    void onUnderflow();

    Config cfg_{};

    XI2s_Tx i2s_{};     // I2S Txの実体
    Regs    regs_{0};   // I2S Txレジスタ
    Regs    regs2_{0};  // クロックMUXレジスタ

    bool     run_{false};    // 初期済みか否か
    uint32_t blk_count_{0};  // AESブロック完了割り込み回数
};
}  // namespace platform
}  // namespace core0
