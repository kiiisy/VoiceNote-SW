/**
 * @file gpio_ps.h
 * @brief PS側のGPIOドライバの薄いラッパ
 *
 * Xilinx の XGpioPs ドライバを C++ クラス化して扱うためのヘッダ。
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xgpiops.h"

namespace core1 {
namespace platform {

/**
 * @brief PS GPIO制御クラス（MIO出力向け）
 *
 * 本クラスは PS 側 GPIO を用いて、MIO ピンに対する
 * 出力レベル制御と出力設定を提供する。
 */
class GpioPs
{
public:
    explicit GpioPs(uint32_t base_addr) : base_addr_(base_addr) {}

    static GpioPs &GetInstance()
    {
        static GpioPs inst;
        return inst;
    }

    int32_t Init();
    void Configure(const uint32_t base_addr) { base_addr_ = base_addr; };
    void WritePin(uint32_t mio, uint32_t level);
    void SetOutput(uint32_t mio);

    GpioPs(const GpioPs &)            = delete;
    GpioPs &operator=(const GpioPs &) = delete;

private:
    GpioPs() = default;
    XGpioPs  gpio_{};     // GPIOドライバの実体
    uint32_t base_addr_;  // ベースアドレス
};
}  // namespace platform
}  // namespace core1
