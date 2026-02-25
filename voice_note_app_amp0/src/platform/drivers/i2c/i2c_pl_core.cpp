/**
 * @file i2c_pl_core.cpp
 * @brief i2c_pl_coreの実装
 */

// 自ヘッダー
#include "i2c_pl_core.h"

// 標準ライブラリ
#include <limits>

// プロジェクトライブラリ
#include "gic_core.h"
#include "logger_core.h"

namespace core0 {
namespace platform {

namespace {

constexpr uint32_t kI2cRetryMax       = 3;        // リトライ上限
constexpr uint32_t kI2cWaitLoopMax    = 1000000;  // 完了待ちループ上限
constexpr uint32_t kWriteRegDataLimit = 32;       // 16bitレジスタ書き込み最大データ長

/**
 * @brief 転送完了フラグが立ち、かつ BUSY 解除されるまで待機する
 *
 * @param iic XIic インスタンス
 * @param done 完了フラグ
 * @param max_loop 待機ループ上限
 * @retval true 待機成功
 * @retval false タイムアウト
 */
bool WaitForTransfer(XIic *iic, volatile bool *done, uint32_t max_loop)
{
    uint32_t timeout = max_loop;
    while (((*done == false) || XIic_IsIicBusy(iic)) && (timeout > 0u)) {
        timeout--;
    }

    return timeout > 0u;
}

/**
 * @brief 完了フラグが立つまで待機する
 *
 * @param done 完了フラグ
 * @param max_loop 待機ループ上限
 * @retval true 待機成功
 * @retval false タイムアウト
 */
bool WaitForFlag(volatile bool *done, uint32_t max_loop)
{
    uint32_t timeout = max_loop;
    while ((*done == false) && (timeout > 0u)) {
        timeout--;
    }

    return timeout > 0u;
}

}  // namespace

/**
 * @brief 送信完了割り込みハンドラ
 * @param ref インスタンス
 */
void I2cPlCore::SendDoneHandler(void *ref) { static_cast<I2cPlCore *>(ref)->SendDone(); }

/**
 * @brief 受信完了割り込みハンドラ
 * @param ref インスタンス
 */
void I2cPlCore::ReceiveDoneHandler(void *ref) { static_cast<I2cPlCore *>(ref)->ReceiveDone(); }

/**
 * @brief ステータス変化割り込みハンドラ
 * @param ref インスタンス
 */
void I2cPlCore::StatusDoneCandler(void *ref) { static_cast<I2cPlCore *>(ref)->StatusDone(); }

/**
 * @brief 送信完了を通知する
 */
void I2cPlCore::SendDone() { transmit_complete_ = true; }

/**
 * @brief 受信完了を通知する
 */
void I2cPlCore::ReceiveDone() { receive_complete_ = true; }

/**
 * @brief ステータス変化通知
 */
void I2cPlCore::StatusDone() {}

/**
 * @brief 割り込みコールバックを再登録する
 */
void I2cPlCore::BindHandlers() noexcept
{
    XIic_SetSendHandler(&iic_, this, reinterpret_cast<XIic_Handler>(&SendDoneHandler));
    XIic_SetRecvHandler(&iic_, this, reinterpret_cast<XIic_Handler>(&ReceiveDoneHandler));
    XIic_SetStatusHandler(&iic_, this, reinterpret_cast<XIic_Handler>(&StatusDoneCandler));
}

/**
 * @brief I2C コアをリセットして復旧する
 *
 * @param reason 復旧理由（ログ用途）
 * @retval true 復旧成功
 * @retval false 復旧失敗
 */
bool I2cPlCore::RecoverBus(const char *reason) noexcept
{
    LOGW("I2C recover: %s", reason);

    XIic_Reset(&iic_);
    XIic_ClearStats(&iic_);
    BindHandlers();

    transmit_complete_ = false;
    receive_complete_  = false;

    if (has_slave_addr_) {
        if (XIic_SetAddress(&iic_, XII_ADDR_TO_SEND_TYPE, slave_addr_) != XST_SUCCESS) {
            LOGE("I2C recover failed: restore slave addr");
            return false;
        }
    }

    return true;
}

/**
 * @brief 初期化処理
 *
 * @return XStatus ステータス（XST_SUCCESS など）
 */
XStatus I2cPlCore::Init(const Config &cfg)
{
    LOG_SCOPE();

    if (initialized_) {
        return XST_SUCCESS;
    }

    cfg_ = cfg;

    auto *xiic_cfg = XIic_LookupConfig(cfg_.base_addr);
    if (!xiic_cfg) {
        LOGE("I2C config not found");
        return XST_DEVICE_NOT_FOUND;
    }

    XStatus status = XIic_CfgInitialize(&iic_, xiic_cfg, xiic_cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        LOGE("I2C initialization failed");
        return status;
    }

    // 割り込み接続
    auto &gic = GicCore::GetInstance();
    status    = gic.Attach(cfg_.irq_id, (Xil_InterruptHandler)XIic_InterruptHandler, (void *)&iic_,
                           GicCore::GicPriority::Normal, GicCore::GicTrigger::Rising);
    if (status != XST_SUCCESS) {
        LOGE("I2C IRQ Attach Failed");
        return status;
    }

    gic.Enable(cfg_.irq_id);

    // 割り込みハンドラの設定
    BindHandlers();

    const XStatus start_status = XIic_Start(&iic_);
    if (start_status != XST_SUCCESS) {
        LOGE("I2C start failed");
        return start_status;
    }

    initialized_ = true;

    return XST_SUCCESS;
}

/**
 * @brief 送信先スレーブアドレスを設定する
 *
 * @param slave_addr 送信先アドレス
 * @return true 設定成功
 * @return false 設定失敗
 */
bool I2cPlCore::SetSlaveAddr(uint16_t slave_addr)
{
    if (slave_addr > 0x7Fu) {
        return false;
    }

    if (XIic_SetAddress(&iic_, XII_ADDR_TO_SEND_TYPE, slave_addr) != XST_SUCCESS) {
        return false;
    }

    slave_addr_     = slave_addr;
    has_slave_addr_ = true;

    return true;
}

/**
 * @brief バイト列を書き込む
 *
 * @param data 送信データ
 * @param len  送信バイト数
 * @return true 成功
 * @return false 失敗（リトライ上限超過）
 */
bool I2cPlCore::Write(const uint8_t *data, size_t len)
{
    if ((data == nullptr) && (len > 0u)) {
        return false;
    }

    for (uint32_t i = 0; i < kI2cRetryMax; ++i) {
        // 1回分の送信トランザクションを開始
        transmit_complete_ = false;
        if (XIic_Start(&iic_) != XST_SUCCESS) {
            (void)RecoverBus("Write start failed");
            continue;
        }
        if (XIic_MasterSend(&iic_, const_cast<uint8_t *>(data), static_cast<int>(len)) != XST_SUCCESS) {
            (void)XIic_Stop(&iic_);
            (void)RecoverBus("Write send failed");
            continue;
        }

        const bool is_wait_ok = WaitForTransfer(&iic_, &transmit_complete_, kI2cWaitLoopMax);
        const bool is_stop_ok = (XIic_Stop(&iic_) == XST_SUCCESS);
        if (is_wait_ok && is_stop_ok && (iic_.Stats.TxErrors == 0)) {
            return true;
        }

        (void)RecoverBus("Write wait/stop failed");
    }

    LOGE("I2C write failed");

    return false;
}

/**
 * @brief バイト列を読み込む
 *
 * @param rx   受信バッファ
 * @param len  受信バイト数
 * @return true 成功
 * @return false 失敗（リトライ上限超過）
 */
bool I2cPlCore::Read(uint8_t *rx, size_t len)
{
    if ((rx == nullptr) && (len > 0u)) {
        return false;
    }

    if (len == 0u) {
        return true;
    }

    for (uint32_t i = 0; i < kI2cRetryMax; ++i) {
        // 1回分の受信トランザクションを開始
        receive_complete_ = false;
        if (XIic_Start(&iic_) != XST_SUCCESS) {
            (void)RecoverBus("Read start failed");
            continue;
        }

        if (XIic_MasterRecv(&iic_, rx, static_cast<int>(len)) != XST_SUCCESS) {
            (void)XIic_Stop(&iic_);
            (void)RecoverBus("Read recv failed");
            continue;
        }

        const bool is_wait_ok = WaitForTransfer(&iic_, &receive_complete_, kI2cWaitLoopMax);
        const bool is_stop_ok = (XIic_Stop(&iic_) == XST_SUCCESS);
        if (is_wait_ok && is_stop_ok && (iic_.Stats.TxErrors == 0)) {
            return true;
        }

        (void)RecoverBus("Read wait/stop failed");
    }

    LOGE("I2C read failed");

    return false;
}

/**
 * @brief STOPを出さずに送信する
 *
 * @param data 送信データ
 * @param len 送信バイト数
 * @retval true 成功
 * @retval false 失敗
 */
bool I2cPlCore::WriteNoStop(const uint8_t *data, size_t len)
{
    if ((data == nullptr) && (len > 0u)) {
        return false;
    }

    if (len == 0u) {
        return true;
    }

    transmit_complete_ = false;
    if (XIic_Start(&iic_) != XST_SUCCESS) {
        (void)RecoverBus("WriteNoStop start failed");
        return false;
    }

    const int sent = XIic_DynMasterSend(&iic_, const_cast<uint8_t *>(data), static_cast<int>(len));
    if (sent != static_cast<int>(len)) {
        (void)XIic_Stop(&iic_);
        (void)RecoverBus("WriteNoStop send failed");
        return false;
    }

    if (!WaitForFlag(&transmit_complete_, kI2cWaitLoopMax)) {
        (void)XIic_Stop(&iic_);
        (void)RecoverBus("WriteNoStop timeout");
        return false;
    }

    return true;
}

/**
 * @brief Repeated START後の受信を行い、最後にSTOPを出す
 *
 * @param rx 受信バッファ
 * @param len 受信バイト数
 * @retval true 成功
 * @retval false 失敗
 */
bool I2cPlCore::ReadWithStop(uint8_t *rx, size_t len)
{
    if ((rx == nullptr) && (len > 0u)) {
        return false;
    }

    if (len == 0u) {
        (void)XIic_Stop(&iic_);
        return true;
    }

    receive_complete_ = false;
    const int recv    = XIic_DynMasterRecv(&iic_, rx, static_cast<int>(len));
    if (recv != static_cast<int>(len)) {
        (void)XIic_Stop(&iic_);
        (void)RecoverBus("ReadWithStop recv failed");
        return false;
    }

    const bool is_wait_ok = WaitForTransfer(&iic_, &receive_complete_, kI2cWaitLoopMax);
    const bool is_stop_ok = (XIic_Stop(&iic_) == XST_SUCCESS);
    if (is_wait_ok && is_stop_ok && (iic_.Stats.TxErrors == 0)) {
        return true;
    }

    (void)RecoverBus("ReadWithStop wait/stop failed");
    return false;
}

/**
 * @brief 1バイトサブアドレス指定の読み出しを行う
 *
 * subaddr を STOP なしで送信し、Repeated START で受信する。
 *
 * @param subaddr サブアドレス（1バイト）
 * @param rx 受信バッファ
 * @param len 受信バイト数
 * @retval true 成功
 * @retval false 失敗
 */
bool I2cPlCore::ReadSubAddr(uint8_t subaddr, uint8_t *rx, size_t len)
{
    if ((rx == nullptr) && (len > 0u)) {
        return false;
    }

    // subaddrをSTOPなしで送る
    if (!WriteNoStop(&subaddr, 1)) {
        return false;
    }

    // Repeated-STARTしてlenバイト受信（最後はSTOP）
    return ReadWithStop(rx, len);
}

}  // namespace platform
}  // namespace core0
