/**
 * @file i2c_pl_core.cpp
 * @brief i2c_pl_coreの実装
 */

// 自ヘッダー
#include "i2c_pl_core.h"

// 標準ライブラリ
#include <cstring>

// プロジェクトライブラリ
#include "gic_core.h"
#include "logger_core.h"

namespace core1 {
namespace platform {

namespace {

constexpr uint32_t kI2cRetryMax       = 3;        // リトライ上限
constexpr uint32_t kI2cWaitLoopMax    = 1000000;  // 完了待ちループ上限
constexpr uint32_t kWriteRegDataLimit = 32;       // 16bitレジスタ書き込み最大データ長

bool WaitForTransfer(XIic *iic, volatile bool *done, uint32_t max_loop)
{
    uint32_t timeout = max_loop;
    while (((*done == false) || XIic_IsIicBusy(iic)) && (timeout > 0u)) {
        timeout--;
    }

    return timeout > 0u;
}

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
void I2cPlCore::StatusDoneHandler(void *ref) { static_cast<I2cPlCore *>(ref)->StatusDone(); }

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

void I2cPlCore::BindHandlers() noexcept
{
    XIic_SetSendHandler(&iic_, this, reinterpret_cast<XIic_Handler>(&SendDoneHandler));
    XIic_SetRecvHandler(&iic_, this, reinterpret_cast<XIic_Handler>(&ReceiveDoneHandler));
    XIic_SetStatusHandler(&iic_, this, reinterpret_cast<XIic_Handler>(&StatusDoneHandler));
}

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

    cfg_ = cfg;

    // ベースアドレスからXIic設定を引いて初期化
    auto *xiic_cfg = XIic_LookupConfig(cfg_.base_addr);
    if (!xiic_cfg) {
        LOGE("I2C config not found");
        return XST_DEVICE_NOT_FOUND;
    }

    XStatus st = XIic_CfgInitialize(&iic_, xiic_cfg, xiic_cfg->BaseAddress);
    if (st != XST_SUCCESS) {
        LOGE("I2C initialization failed");
        return st;
    }

    // 割り込み登録
    auto &gic = GicCore::GetInstance();
    // AXI IIC の割り込み出力はレベルで保持されるため、LevelHigh で受ける。
    // Edge設定だと高負荷時に完了IRQを取りこぼし、送信フェーズ待ちがタイムアウトしやすい。
    st        = gic.Attach(cfg_.irq_id, (Xil_InterruptHandler)XIic_InterruptHandler, (void *)&iic_,
                           GicCore::GicPriority::Normal, GicCore::GicTrigger::LevelHigh);
    if (st != XST_SUCCESS) {
        LOGE("I2C IRQ Attach Failed");
        return st;
    }

    gic.Enable(cfg_.irq_id);

    // 割り込みハンドラの設定
    BindHandlers();

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
    // 範囲チェック
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
        XIic_ClearStats(&iic_);
        // 1回分の送信トランザクションを開始
        transmit_complete_ = false;
        if (XIic_Start(&iic_) != XST_SUCCESS) {
            continue;
        }
        if (XIic_MasterSend(&iic_, const_cast<uint8_t *>(data), static_cast<int>(len)) != XST_SUCCESS) {
            (void)XIic_Stop(&iic_);
            continue;
        }

        const bool is_wait_ok = WaitForTransfer(&iic_, &transmit_complete_, kI2cWaitLoopMax);
        const bool is_stop_ok = (XIic_Stop(&iic_) == XST_SUCCESS);
        if (is_wait_ok && is_stop_ok && (iic_.Stats.TxErrors == 0)) {
            return true;
        }
    }

    LOGE("I2C write failed");

    return false;
}

/**
 * @brief バイト列を読み込む
 *
 * @param rx 受信バッファ
 * @param len  受信バイト数
 * @return true 成功
 * @return false 失敗（リトライ上限超過）
 */
bool I2cPlCore::Read(uint8_t *rx, size_t len)
{
    if ((rx == nullptr) && (len > 0u)) {
        return false;
    }

    for (uint32_t i = 0; i < kI2cRetryMax; ++i) {
        XIic_ClearStats(&iic_);
        // 1回分の受信トランザクションを開始
        receive_complete_ = false;
        if (XIic_Start(&iic_) != XST_SUCCESS) {
            continue;
        }

        if (XIic_MasterRecv(&iic_, rx, static_cast<int>(len)) != XST_SUCCESS) {
            (void)XIic_Stop(&iic_);
            continue;
        }

        const bool is_wait_ok = WaitForTransfer(&iic_, &receive_complete_, kI2cWaitLoopMax);
        const bool is_stop_ok = (XIic_Stop(&iic_) == XST_SUCCESS);
        if (is_wait_ok && is_stop_ok && (iic_.Stats.TxErrors == 0)) {
            return true;
        }
    }

    LOGE("I2C read failed");

    return false;
}

/**
 * @brief レジスタを指定して読み出す
 *
 * @param reg 16bitレジスタアドレス（ビックエンディアンで送信）
 * @param buf 受信バッファ
 * @param len 受信バイト数
 * @return true 成功
 * @return false 失敗（Start/Send/Recv/Timeout/Retry超過）
 */
bool I2cPlCore::ReadReg(uint16_t reg, uint8_t *buf, size_t len)
{
    if ((buf == nullptr) && (len > 0u)) {
        return false;
    }

    XIic_ClearStats(&iic_);

    uint8_t sub[2] = {uint8_t(reg >> 8), uint8_t(reg)};

    // 送信フェーズ
    transmit_complete_ = 0;
    if (XIic_Start(&iic_) != XST_SUCCESS) {
        (void)RecoverBus("ReadReg start failed");
        return false;
    }

    if (XIic_MasterSend(&iic_, sub, 2) != XST_SUCCESS) {
        XIic_Stop(&iic_);
        (void)RecoverBus("ReadReg subaddr send failed");
        return false;
    }

    // 送信完了待ち
    uint32_t       timeout     = kI2cWaitLoopMax;
    uint32_t       retries     = 0;
    const uint32_t max_retries = kI2cRetryMax;

    while ((!transmit_complete_ || XIic_IsIicBusy(&iic_)) && timeout > 0) {
        if (iic_.Stats.TxErrors) {  // NACKなど
            if (++retries > max_retries) {
                LOGE("I2C Write Phase Retry Limit Exceeded");
                XIic_Stop(&iic_);
                (void)RecoverBus("ReadReg write-phase retry exceeded");
                return false;
            }
            // リトライ
            XIic_ClearStats(&iic_);
            XIic_Stop(&iic_);
            if (XIic_Start(&iic_) != XST_SUCCESS) {
                (void)RecoverBus("ReadReg retry start failed");
                return false;
            }
            if (!XIic_IsIicBusy(&iic_) && XIic_MasterSend(&iic_, sub, 2) == XST_SUCCESS) {
                XIic_ClearStats(&iic_);
            }
        }
        timeout--;
    }

    if (timeout <= 0) {
        LOGE("I2C Write Phase Timeout");
        XIic_Stop(&iic_);
        (void)RecoverBus("ReadReg write-phase timeout");
        return false;
    }

    if (XIic_Stop(&iic_) != XST_SUCCESS) {
        (void)RecoverBus("ReadReg stop after write-phase failed");
        return false;
    }

    // 受信フェーズ
    receive_complete_ = 0;
    if (XIic_Start(&iic_) != XST_SUCCESS) {
        (void)RecoverBus("ReadReg read-phase start failed");
        return false;
    }

    if (XIic_MasterRecv(&iic_, buf, static_cast<int>(len)) != XST_SUCCESS) {
        XIic_Stop(&iic_);
        (void)RecoverBus("ReadReg read-phase recv failed");
        return false;
    }

    timeout = kI2cWaitLoopMax;
    while ((!receive_complete_ || XIic_IsIicBusy(&iic_)) && timeout > 0) {
        timeout--;
    }

    if (timeout <= 0) {
        LOGE("I2C Read Phase Timeout");
        XIic_Stop(&iic_);
        (void)RecoverBus("ReadReg read-phase timeout");
        return false;
    }

    if (XIic_Stop(&iic_) != XST_SUCCESS) {
        (void)RecoverBus("ReadReg final stop failed");
        return false;
    }

    return true;
}

/**
 * @brief レジスタを指定して書き込む
 *
 * @param reg  16bitレジスタアドレス（ビックエンディアンで送信）
 * @param data 書き込むデータ
 * @param len  書き込みバイト数（最大 32）
 * @return true 成功
 * @return false 失敗（len 超過 / Start/Send/Timeout/Retry超過）
 */
bool I2cPlCore::WriteReg(uint16_t reg, const uint8_t *data, size_t len)
{
    if ((data == nullptr) && (len > 0u)) {
        return false;
    }
    if (len > kWriteRegDataLimit) {
        return false;
    }

    XIic_ClearStats(&iic_);

    uint8_t frame[2 + kWriteRegDataLimit];
    frame[0] = static_cast<uint8_t>(reg >> 8);
    frame[1] = static_cast<uint8_t>(reg & 0xFF);
    if (len) {
        memcpy(&frame[2], data, len);
    }

    // 送信フェーズ
    transmit_complete_ = 0;

    if (XIic_Start(&iic_) != XST_SUCCESS) {
        (void)RecoverBus("WriteReg start failed");
        return false;
    }

    if (XIic_MasterSend(&iic_, frame, static_cast<int>(2 + len)) != XST_SUCCESS) {
        XIic_Stop(&iic_);
        (void)RecoverBus("WriteReg frame send failed");
        return false;
    }

    // 送信完了待ち（NACKなどが出たらやり直す）
    uint32_t  timeout     = kI2cWaitLoopMax;
    uint32_t  retries     = 0;
    const uint32_t max_retries = kI2cRetryMax;
    while ((!transmit_complete_ || XIic_IsIicBusy(&iic_)) && timeout > 0) {
        if (iic_.Stats.TxErrors) {
            if (++retries > max_retries) {
                LOGE("I2C WriteReg Retry Limit Exceeded");
                XIic_Stop(&iic_);
                (void)RecoverBus("WriteReg retry exceeded");
                return false;
            }
            // NACKなどの再送
            XIic_ClearStats(&iic_);
            XIic_Stop(&iic_);
            if (XIic_Start(&iic_) != XST_SUCCESS) {
                (void)RecoverBus("WriteReg retry start failed");
                return false;
            }
            if (!XIic_IsIicBusy(&iic_) && XIic_MasterSend(&iic_, frame, static_cast<int>(2 + len)) == XST_SUCCESS) {
                XIic_ClearStats(&iic_);
            }
        }
        timeout--;
    }

    if (timeout <= 0) {
        LOGE("I2C WriteReg Timeout");
        XIic_Stop(&iic_);
        (void)RecoverBus("WriteReg timeout");
        return false;
    }

    if (XIic_Stop(&iic_) != XST_SUCCESS) {
        (void)RecoverBus("WriteReg final stop failed");
        return false;
    }

    return true;
}

}  // namespace platform
}  // namespace core1
