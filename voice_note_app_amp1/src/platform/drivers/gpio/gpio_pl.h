/**
 * @file gpio_pl.h
 * @brief AXI GPIOドライバ定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xgpio.h"
#include "xiltimer.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xstatus.h"

// プロジェクトライブラリ
#include "gpio_if.h"

namespace core1 {
namespace platform {

/**
 * @class GpioPl
 * @brief PL側のAXI GPIOを扱うGPIOドライバ
 *
 * - 入力チャネル(ch_0)：割り込み対応
 * - 出力チャネル(ch_2)：GPIO出力
 */
class GpioPl final : public GpioIf
{
public:
    enum ChType : uint32_t
    {
        kChIn = 0x01,
        kChOut,
    };

    struct Config
    {
        uint32_t addr;    // AXI GPIO Device ID
        uint32_t irq_id;  // GIC IRQ ID
        uint32_t ch_0;    // 入力チャネル番号
        uint32_t ch_2;    // 出力チャネル番号
    };

    explicit GpioPl(const Config &c) : cfg_(c) {}

    bool Init() override;
    void Deinit() override;

    void     SetDir(uint32_t pin, GpioDir d) override;
    uint32_t ReadPin(uint32_t pin) override;
    void     WritePin(uint32_t pin, uint32_t v) override;

    uint32_t ReadInputs() override;
    void     WriteOutputs(uint32_t mask) override;

    bool EnableIrq() override;
    void DisableIrq() override;

    void SetIrqCallback(GpioIrqCallback cb) override { cb_ = std::move(cb); }

    void MaskIrq() { XGpio_InterruptDisable(&inst_, cfg_.ch_0); }
    void UnmaskIrq() { XGpio_InterruptEnable(&inst_, cfg_.ch_0); }
    void EnableIrqLight();

private:
    const uint32_t in_setting_  = 0xFFFFFFFF;
    const uint32_t out_setting_ = 0x00000000;

    static void IsrThunk(void *ref);
    void        OnIsr();

    XGpio           inst_{};                // AXI GPIOの実体
    Config          cfg_{};                 // 設定
    bool            irq_attached_ = false;  // GICに接続済み
    GpioIrqCallback cb_{};                  // 割り込みコールバック
};
}  // namespace platform
}  // namespace core1
