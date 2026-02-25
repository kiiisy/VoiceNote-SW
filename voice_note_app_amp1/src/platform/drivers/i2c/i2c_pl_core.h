/**
 * @file i2c_pl_core.h
 * @brief PL側のAXI IICを扱うI2Cコアクラス定義
 */
#pragma once

// 標準ライブラリ
#include <cstddef>
#include <cstdint>

// Vitisライブラリ
#include "xiic.h"
#include "xstatus.h"

namespace core1 {
namespace platform {

/**
 * @brief PL側のAXI IICを制御するクラス
 */
class I2cPlCore
{
public:
    struct Config
    {
        uint32_t base_addr{};
        uint32_t irq_id{};
    };

    static I2cPlCore &GetInstance()
    {
        static I2cPlCore inst;
        return inst;
    }

    XStatus Init(const Config &cfg);

    bool SetSlaveAddr(uint16_t slave_addr);

    bool Write(const uint8_t *data, size_t len);
    bool Read(uint8_t *rx, size_t len);
    bool WriteReg(uint16_t reg, const uint8_t *data, size_t len);
    bool ReadReg(uint16_t reg, uint8_t *buf, size_t len);

    void GpioOutput(uint8_t value) { XIic_SetGpOutput(&iic_, value); }

private:
    I2cPlCore() = default;
    ~I2cPlCore() { XIic_Stop(&iic_); }

    void BindHandlers() noexcept;
    bool RecoverBus(const char *reason) noexcept;

    // コールバック系
    static void SendDoneHandler(void *ref);
    static void ReceiveDoneHandler(void *ref);
    static void StatusDoneHandler(void *ref);
    void        SendDone();
    void        ReceiveDone();
    void        StatusDone();

    XIic          iic_{};                     // IICの実体
    Config        cfg_{};                     // 初期化設定
    volatile bool transmit_complete_{false};  // 送信完了フラグ
    volatile bool receive_complete_{false};   // 受信完了フラグ
    uint16_t      slave_addr_{0};             // 現在のスレーブアドレス
    bool          has_slave_addr_{false};     // スレーブアドレス設定済みか
};

}  // namespace platform
}  // namespace core1
