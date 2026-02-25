#pragma once
#include "gpio_if.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace core0 {
namespace platform {

// Board 上のボタン/LED をシンプル API で扱うラッパ
class BoardIo final
{
public:
    BoardIo() = default;
    ~BoardIo() { Deinit(); }

    bool Init(GpioIf *port, uint32_t btn_bits_mask, uint32_t led_bits_mask);
    void Deinit();

    // LED 操作
    void SetLed(uint32_t index, bool on);
    void ToggleLed(uint32_t index);
    void WriteLeds(uint32_t mask);  // led_bits_mask 範囲内で上書き

    // ボタン読み（ポーリング）
    bool ReadButtons(uint32_t *out_state, uint32_t *out_changed);

    // 割り込み通知（変化あったピンごとに呼ばれる）
    void SetButtonCallback(std::function<void(uint32_t pin, bool level)> cb);
    bool EnableButtonIrq();
    void DisableButtonIrq();

private:
    GpioIf  *port_     = nullptr;  // 非所有
    uint32_t btn_mask_ = 0;        // ボタンが乗る論理ビット集合
    uint32_t led_mask_ = 0;        // LED が乗る論理ビット集合
    uint32_t btn_last_ = 0;        // 直近のボタン状態（ポーリング用）
    uint32_t led_last_ = 0;
};
}  // namespace platform
}  // namespace core0
