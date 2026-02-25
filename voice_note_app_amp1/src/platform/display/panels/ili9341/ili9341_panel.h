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
    void Init(const LcdBus &bus) override;
    void Reset(const LcdBus &bus) override;
    void SetRotation(const LcdBus &bus, uint8_t madctl) override;
};
}  // namespace platform
}  // namespace core1
