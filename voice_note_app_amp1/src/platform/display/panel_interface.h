/**
 * @file panel_interface.h
 * @brief LCDパネル抽象インタフェース
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "gpio_ps.h"
#include "lcd_bus.h"

namespace core1 {
namespace platform {

/**
 * @brief LCDパネル制御の抽象インタフェース
 */
class PanelInterface
{
public:
    virtual ~PanelInterface()                                   = default;
    virtual void Init(const LcdBus &bus)                        = 0;
    virtual void Reset(const LcdBus &bus)                       = 0;
    virtual void SetRotation(const LcdBus &bus, uint8_t madctl) = 0;
    virtual void SetAddrWindow(const LcdBus &bus, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) noexcept = 0;

    /**
     * @brief MADCTL ビット定義
     */
    enum : uint8_t
    {
        MADCTL_MY  = 0x80,  // Row address order（Y 反転）
        MADCTL_MX  = 0x40,  // Column address order（X 反転）
        MADCTL_MV  = 0x20,  // Row/Column exchange（X/Y 入替）
        MADCTL_ML  = 0x10,  // Vertical refresh order（コントローラ依存）
        MADCTL_BGR = 0x08,  // Color order（BGR）
        MADCTL_MH  = 0x04,  // Horizontal refresh order（コントローラ依存）
    };
};

}  // namespace platform
}  // namespace core1
