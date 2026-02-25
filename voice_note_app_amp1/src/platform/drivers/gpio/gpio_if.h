/**
 * @file gpio_if.h
 * @brief GPIO抽象インターフェース定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>
#include <functional>

namespace core1 {
namespace platform {

/**
 * @enum GpioDir
 * @brief GPIOピン方向
 */
enum class GpioDir
{
    Out,  // 出力
    In,   // 入力
};

using GpioIrqCallback = std::function<void(void)>;

/**
 * @struct GpioIf
 * @brief GPIO抽象インターフェース
 *
 * - PS GPIO / PL AXI GPIO の差異を吸収するための共通IF
 * - 初期化、入出力、割り込み制御を定義する
 */
struct GpioIf
{
    virtual ~GpioIf()     = default;
    virtual bool Init()   = 0;
    virtual void Deinit() = 0;

    virtual void     SetDir(uint32_t pin, GpioDir d)    = 0;
    virtual uint32_t ReadPin(uint32_t pin)              = 0;
    virtual void     WritePin(uint32_t pin, uint32_t v) = 0;

    virtual uint32_t ReadInputs()                = 0;
    virtual void     WriteOutputs(uint32_t mask) = 0;

    virtual bool EnableIrq()                        = 0;
    virtual void DisableIrq()                       = 0;
    virtual void SetIrqCallback(GpioIrqCallback cb) = 0;
};
}  // namespace platform
}  // namespace core1
