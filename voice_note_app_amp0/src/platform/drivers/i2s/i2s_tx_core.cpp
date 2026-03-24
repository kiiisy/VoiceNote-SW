/**
 * @file i2s_tx_core.cpp
 * @brief I2S Transmitterの実装
 */

// 自ヘッダー
#include "i2s_tx_core.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace platform {

/**
 * @brief AESブロック完了割り込みのエントリ
 */
void I2sTx::AesBlockCmplHandlerEntry(void *ref) { static_cast<I2sTx *>(ref)->onAesBlockComplete(); }

/**
 * @brief AESブロックエラー割り込みのエントリ
 */
void I2sTx::AesBlockErrHandlerEntry(void *ref) { static_cast<I2sTx *>(ref)->onAesBlockError(); }

/**
 * @brief AESチャネルステータス更新割り込みのエントリ
 */
void I2sTx::AesChStatHandlerEntry(void *ref) { static_cast<I2sTx *>(ref)->onAesChStatus(); }

/**
 * @brief オーディオアンダーフロー割り込みのエントリ
 */
void I2sTx::UnderflowHandlerEntry(void *ref) { static_cast<I2sTx *>(ref)->onUnderflow(); }

/**
 * @brief AESブロック完了時の処理
 *
 * 送信が進行していることを示すイベント。
 * 進行確認（再生が動いているか）やデバッグ用にカウントする。
 */
void I2sTx::onAesBlockComplete() { ++blk_count_; }

/**
 * @brief AESブロックエラー発生時の処理
 *
 * AES フレーム同期の異常。ストリーム供給異常や設定不一致が疑われる。
 * 必要に応じてログ出力、状態取得、復旧処理を実装する。
 */
void I2sTx::onAesBlockError() { /* ログ等 */ }

/**
 * @brief AESチャネルステータス更新時の処理
 *
 * ステータスビットの更新通知。必要があれば取得/転送などを実装する。
 */
void I2sTx::onAesChStatus() { /* 必要なら */ }

/**
 * @brief オーディオアンダーフロー発生時の処理
 *
 * I2S へ供給するサンプルが間に合わなかったことを示す。
 * 上流（DMA/Audio Formatter 等）の供給を見直す必要がある。
 */
void I2sTx::onUnderflow() { /* バッファ補充など */ }

/**
 * @brief I2S Tx IPを初期化する
 *
 * - ドライバ初期化（XI2s_Tx_CfgInitialize）
 * - GIC へ割り込み接続（XI2s_Tx_IntrHandler）
 * - 各イベントハンドラを this に接続（XI2s_Tx_SetHandler）
 * - 割り込みソースを有効化（XI2s_Tx_IntrEnable）
 * - Validity 設定、チャネル MUX 設定、SCLK 分周設定
 *
 * @retval XST_SUCCESS 成功
 * @retval XST_FAILURE 失敗（LookupConfig 失敗など）
 */
XStatus I2sTx::Init(const Config &cfg)
{
    LOG_SCOPE();

    if (run_) {
        LOGI("I2s Tx Already Started");
        return XST_SUCCESS;
    }

    if (cfg.base_addr == 0u || cfg.mux_base_addr == 0u) {
        LOGE("I2s Tx invalid config");
        return XST_FAILURE;
    }

    cfg_ = cfg;

    regs_.SetBase(cfg_.base_addr);
    regs2_.SetBase(cfg_.mux_base_addr);

    auto *hw_cfg = XI2s_Tx_LookupConfig(cfg_.base_addr);
    if (!hw_cfg) {
        LOGE("I2s Tx LookupConfig Failed");
        return XST_FAILURE;
    }

    XStatus st = XI2s_Tx_CfgInitialize(&i2s_, hw_cfg, hw_cfg->BaseAddress);
    if (st != XST_SUCCESS) {
        LOGE("I2s Tx CfgInitialize Failed");
        return st;
    }

    // IRQ配線
    auto &gic = GicCore::GetInstance();
    st = gic.Attach(cfg_.irq_id, (Xil_InterruptHandler)XI2s_Tx_IntrHandler, (void *)&i2s_, GicCore::GicPriority::Normal,
                    GicCore::GicTrigger::Rising);
    if (st != XST_SUCCESS) {
        LOGE("I2s Tx IRQ Attach Failed");
        return st;
    }
    gic.Enable(cfg_.irq_id);

    // 自デバイスのイベント→this へ
    XI2s_Tx_SetHandler(&i2s_, XI2S_TX_HANDLER_AES_BLKCMPLT, &I2sTx::AesBlockCmplHandlerEntry, this);
    XI2s_Tx_SetHandler(&i2s_, XI2S_TX_HANDLER_AES_BLKSYNCERR, &I2sTx::AesBlockErrHandlerEntry, this);
    XI2s_Tx_SetHandler(&i2s_, XI2S_TX_HANDLER_AES_CHSTSUPD, &I2sTx::AesChStatHandlerEntry, this);
    XI2s_Tx_SetHandler(&i2s_, XI2S_TX_HANDLER_AUD_UNDRFLW, &I2sTx::UnderflowHandlerEntry, this);

    // 割り込み有効化
    XI2s_Tx_IntrEnable(&i2s_, XI2S_TX_GINTR_EN_MASK | XI2S_TX_INTR_AES_BLKCMPLT_MASK | XI2S_TX_INTR_AES_CHSTSUPD_MASK |
                                  XI2S_TX_INTR_AUDUNDRFLW_MASK);

    regs_.VALIDITY_REG().Write(1);
    XI2s_Tx_SetChMux(&i2s_, XI2S_TX_CHID0, XI2S_TX_CHMUX_AXIS_01);
    // こっち側不要かも
    XI2s_Tx_SetChMux(&i2s_, XI2S_TX_CHID1, XI2S_TX_CHMUX_AXIS_01);

    XI2s_Tx_SetSclkOutDiv(&i2s_, i2stx::kI2sTxMclk, i2stx::kI2sTxfSKhz);

    run_ = true;
    return XST_SUCCESS;
}

/**
 * @brief I2S Tx IP を終了する
 *
 * - 送信停止
 * - GIC から割り込みを無効化/デタッチ
 */
void I2sTx::Deinit()
{
    if (!run_) {
        LOGW("I2s Tx Not Started");
        return;
    }

    Disable();

    auto &gic = GicCore::GetInstance();
    gic.Disable(cfg_.irq_id);
    gic.Detach(cfg_.irq_id);

    run_ = false;
}

/**
 * @brief I2S Tx の送信を開始する
 *
 * - デバッグ用にレジスタをダンプ
 * - Clock MUX を Tx 側へ切替（Write(0x01)）
 * - CONTROL の Enable ビットをセット
 *
 * @retval XST_SUCCESS 成功
 * @retval XST_FAILURE 未初期化状態
 */
XStatus I2sTx::Enable()
{
    if (!run_) {
        LOGW("I2s Tx Not Started");
        return XST_FAILURE;
    }

    DumpI2STxRegs();

    regs2_.MUX_CONTROL_REG().Write(0x01);
    regs_.CONTROL_REG().SetBits(i2stx::CTRL_EN_MASK);

    LOGI("I2s TX start");

    return XST_SUCCESS;
}

/**
 * @brief I2S Tx の送信を停止する
 *
 * CONTROL の Enable ビットをクリアする。
 */
void I2sTx::Disable()
{
    if (!run_) {
        LOGW("I2s Tx Not Started");
        return;
    }

    regs_.CONTROL_REG().ClrBits(i2stx::CTRL_EN_MASK);

    LOGI("I2s TX done");
}

/**
 * @brief I2S Tx 関連レジスタをダンプする（デバッグ用）
 *
 * 通信・設定状態の確認用途。
 * - CONTROL / VALIDITY / IRQ Enable/Status
 * - 分周設定、チャネル設定
 * - AES ステータス
 * - Clock MUX 状態
 */
void I2sTx::DumpI2STxRegs()
{
    uint32_t ctrl = regs_.CONTROL_REG().Read();
    uint32_t vld  = regs_.VALIDITY_REG().Read();
    uint32_t ien  = regs_.IRQ_ENABLE_REG().Read();
    uint32_t ist  = regs_.IRQ_STATUS_REG().Read();
    uint32_t div  = regs_.SCLK_DIV_REG().Read();
    uint32_t ch01 = regs_.CH_CTRL_01_REG().Read();
    uint32_t ch23 = regs_.CH_CTRL_23_REG().Read();
    uint32_t ch45 = regs_.CH_CTRL_45_REG().Read();
    uint32_t ch67 = regs_.CH_CTRL_67_REG().Read();
    uint32_t aes0 = regs_.AES_STATUS_0_REG().Read();
    uint32_t aes1 = regs_.AES_STATUS_1_REG().Read();
    uint32_t aes2 = regs_.AES_STATUS_2_REG().Read();
    uint32_t aes3 = regs_.AES_STATUS_3_REG().Read();
    uint32_t aes4 = regs_.AES_STATUS_4_REG().Read();
    uint32_t aes5 = regs_.AES_STATUS_5_REG().Read();
    uint32_t mux  = regs2_.MUX_CONTROL_REG().Read();

    xil_printf("[I2S] CTRL=%08X VLD=%08X IEN=%08X IST=%08X\n", ctrl, vld, ien, ist);
    xil_printf("[I2S] DIV =%08X MUX=%08X\n", div, mux);
    xil_printf("[I2S] CH01=%08X CH23=%08X CH45=%08X CH67=%08X\n", ch01, ch23, ch45, ch67);
    xil_printf("[I2S] AES0=%08X AES1=%08X AES2=%08X AES3=%08X AES4=%08X AES5=%08X\n", aes0, aes1, aes2, aes3, aes4,
               aes5);
}

}  // namespace platform
}  // namespace core0
