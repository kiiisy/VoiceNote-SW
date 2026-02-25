#pragma once
#include <cstdint>
#include <functional>

// 汎用 GPIO インターフェース（PS/AXI の差を吸収）
// - pin は "論理ビット番号" として扱う（BoardIo は 0..N でアクセス）
// - 実デバイス（PS=全MIO/AXI=チャネル内bit）への割付は各実装で吸収

namespace core0 {
namespace platform {

enum class GpioDir
{
    Out,
    In,
};

using GpioIrqCallback = std::function<void(uint32_t pin, bool level)>;

struct GpioIf
{
    virtual ~GpioIf()     = default;
    virtual bool Init()   = 0;
    virtual void Deinit() = 0;

    virtual void     SetDir(uint32_t pin, GpioDir d)    = 0;
    virtual uint32_t ReadPin(uint32_t pin)              = 0;  // 0/1 を返す
    virtual void     WritePin(uint32_t pin, uint32_t v) = 0;  // 0/1

    // まとめ読み/書き（必要なら実装側で高速化）
    virtual uint32_t ReadInputs()                = 0;  // 入力ビット集合
    virtual void     WriteOutputs(uint32_t mask) = 0;  // 出力ビット集合を一括設定

    // 割り込み（ピン単位。AXI の場合はボタン用チャネル内のbit）
    virtual bool EnableIrq()                        = 0;
    virtual void DisableIrq()                       = 0;
    virtual void SetIrqCallback(GpioIrqCallback cb) = 0;
};
}  // namespace platform
}  // namespace core0
