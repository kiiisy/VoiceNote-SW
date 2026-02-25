/**
 * @file i2s_rx_core.cpp
 * @brief I2S Receiverの実装
 */

// 自ヘッダー
#include "i2s_rx_core.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace platform {

/**
 * @brief AES ブロック完了割り込み ISR エントリ
 */
void I2sRx::AesBlockCmplHandlerEntry(void *ref) { static_cast<I2sRx *>(ref)->onAesBlockComplete(); }

/**
 * @brief オーディオオーバーフロー割り込み ISR エントリ
 */
void I2sRx::OverflowHandlerEntry(void *ref) { static_cast<I2sRx *>(ref)->onOverflow(); }

/**
 * @brief AES ブロック完了時の処理
 *
 * 1 ブロック分の AES データ受信完了をカウントする。
 */
void I2sRx::onAesBlockComplete() { ++blk_count_; }

/**
 * @brief オーディオオーバーフロー発生時の処理
 *
 * バッファ不足やデータ取りこぼし発生時に呼ばれる。
 * 必要に応じてエラーカウントや復旧処理を実装する。
 */
void I2sRx::onOverflow() { /* バッファ補充など */ }

/**
 * @brief I2S Rx IP を初期化する
 *
 * - ドライバ初期化
 * - GIC 割り込み接続
 * - AES ブロック完了 / オーバーフロー割り込み設定
 *
 * @retval XST_SUCCESS 成功
 * @retval XST_FAILURE 失敗
 */
XStatus I2sRx::Init(const Config &cfg)
{
    LOG_SCOPE();

    if (run_) {
        LOGI("I2s Rx Already Started");
        return XST_SUCCESS;
    }

    if (cfg.base_addr == 0u || cfg.mux_base_addr == 0u) {
        LOGE("I2s Rx invalid config");
        return XST_FAILURE;
    }

    cfg_ = cfg;

    regs_.SetBase(cfg_.base_addr);
    regs2_.SetBase(cfg_.mux_base_addr);

    auto *hw_cfg = XI2s_Rx_LookupConfig(cfg_.base_addr);
    if (!hw_cfg) {
        LOGE("I2s Rx LookupConfig Failed");
        return XST_FAILURE;
    }

    XStatus status = XI2s_Rx_CfgInitialize(&i2s_, hw_cfg, hw_cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        LOGE("I2s Rx CfgInitialize Failed");
        return status;
    }

    // IRQ配線
    auto &gic = GicCore::GetInstance();
    status = gic.Attach(cfg_.irq_id, (Xil_InterruptHandler)XI2s_Rx_IntrHandler, (void *)&i2s_, GicCore::GicPriority::Normal,
                        GicCore::GicTrigger::Rising);
    if (status != XST_SUCCESS) {
        LOGE("I2s Rx IRQ Attach Failed");
        return status;
    }

    gic.Enable(cfg_.irq_id);

    // 自デバイスのイベント→this へ
    XI2s_Rx_SetHandler(&i2s_, XI2S_RX_HANDLER_AES_BLKCMPLT, &I2sRx::AesBlockCmplHandlerEntry, this);
    XI2s_Rx_SetHandler(&i2s_, XI2S_RX_HANDLER_AUD_OVRFLW, &I2sRx::OverflowHandlerEntry, this);

    // 割り込み有効化
    XI2s_Rx_IntrEnable(&i2s_, XI2S_RX_GINTR_EN_MASK | XI2S_RX_INTR_AES_BLKCMPLT_MASK | XI2S_RX_INTR_AUDOVRFLW_MASK);

    XI2s_Rx_SetChMux(&i2s_, XI2S_RX_CHID0, XI2S_RX_CHMUX_XI2S_01);
    // こっち側不要かも
    //XI2s_Rx_SetChMux(&i2s_, XI2S_RX_CHID1, XI2S_RX_CHMUX_XI2S_01);

    XI2s_Rx_SetSclkOutDiv(&i2s_, i2stx::kI2sRxMclk, i2stx::kI2sRxFsKhz);

    run_ = true;
    return XST_SUCCESS;
}

/**
 * @brief I2S Rx IP を終了する
 */
void I2sRx::Deinit()
{
    if (!run_) {
        LOGW("I2s Rx Not Started");
        return;
    }

    Disable();

    auto &gic = GicCore::GetInstance();
    gic.Disable(cfg_.irq_id);
    gic.Detach(cfg_.irq_id);

    run_ = false;
}

/**
 * @brief I2S Rx の受信を開始する
 */
XStatus I2sRx::Enable()
{
    if (!run_) {
        LOGW("I2s Rx Not Started");
        return XST_FAILURE;
    }
    regs2_.MUX_CONTROL_REG().Write(0x00);
    regs_.CONTROL_REG().SetBits(i2stx::CTRL_EN_MASK);

    LOGD("I2s RX start");

    return XST_SUCCESS;
}

/**
 * @brief I2S Rx の受信を停止する
 */
void I2sRx::Disable()
{
    if (!run_) {
        LOGW("I2s Rx Not Started");
        return;
    }

    regs_.CONTROL_REG().ClrBits(i2stx::CTRL_EN_MASK);

    LOGD("I2s RX done");
}

}  // namespace platform
}  // namespace core0
