#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "panel_interface.h"

namespace core1 {
namespace platform {

class Ili9341Panel final : public PanelInterface
{
public:
    void Init(const LcdBus &bus) noexcept override;
    void Reset(const LcdBus &bus) noexcept override;
    void SetRotation(const LcdBus &bus, uint8_t madctl) noexcept override;
    void SetAddrWindow(const LcdBus &bus, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) noexcept override;
};
}  // namespace platform
}  // namespace core1
