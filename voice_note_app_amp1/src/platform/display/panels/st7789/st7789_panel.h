/**
 * @file st7789_panel.h
 * @brief ST7789制御クラス定義
 */
#pragma once

// プロジェクトライブラリ
#include "display_spec.h"
#include "panel_interface.h"

namespace core1 {
namespace platform {

class LcdBus;

/**
 * @brief ST7789制御クラス
 */
class St7789Panel final : public PanelInterface
{
public:
    void Init(const LcdBus &bus) noexcept override;
    void Reset(const LcdBus &bus) noexcept override;
    void SetRotation(const LcdBus &bus, uint8_t madctl) noexcept override;
    void SetAddrWindow(const LcdBus &b, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) noexcept override;

    bool SelfTestPattern(LcdBus &b, bool use_bgr = false) noexcept;

private:
    static constexpr uint8_t CMD_SWRESET  = 0x01;  // ソフトウェアリセット
    static constexpr uint8_t CMD_SLP_OUT  = 0x11;  // スリープアウト
    static constexpr uint8_t CMD_DISP_ON  = 0x29;  // ディスプレイオン
    static constexpr uint8_t CMD_MADCTL   = 0x36;  // メモリデータアクセス制御
    static constexpr uint8_t CMD_COLMOD   = 0x3A;  // pixフォーマット
    static constexpr uint8_t CMD_CASET    = 0x2A;  // Column address set
    static constexpr uint8_t CMD_RASET    = 0x2B;  // Row address set
    static constexpr uint8_t CMD_RAMWR    = 0x2C;  // メモリ書き込み
    static constexpr uint8_t CMD_INVON    = 0x21;  // ディスプレイ反転ON
    static constexpr uint8_t CMD_INVOFF   = 0x20;  // ディスプレイ反転OFF
    static constexpr uint8_t COLMOD_16BPP = 0x55;  // RGB565

    static void CMD(const LcdBus &b, uint8_t c) noexcept;
    static void DAT(const LcdBus &b, uint8_t d) noexcept;
    static void DAT(const LcdBus &b, const uint8_t *p, size_t n) noexcept;

    static void SetFullAddr(const LcdBus &b) noexcept;

    static constexpr uint16_t PHYS_W = common::display::kWidth;   // 物理X範囲
    static constexpr uint16_t PHYS_H = common::display::kHeight;  // 物理Y範囲
};

}  // namespace platform
}  // namespace core1
