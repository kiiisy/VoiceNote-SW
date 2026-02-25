/**
 * @file st7789_panel.cpp
 * @brief st7789_panelの実装
 */

// 自ヘッダー
#include "st7789_panel.h"

// 標準ライブラリ
#include <algorithm>

// Vitisライブラリ
#include "lcd_bus.h"

// プロジェクトライブラリ
#include "sleep.h"

namespace core1 {
namespace platform {

/**
 * @brief 1バイトコマンド送信ヘルパ
 * @param bus LcdBus
 * @param cmd コマンド
 */
void St7789Panel::CMD(const LcdBus &bus, uint8_t cmd) noexcept { bus.Cmd(cmd); }

/**
 * @brief 1バイトデータ送信ヘルパ
 * @param bus LcdBus
 * @param data データ
 */
void St7789Panel::DAT(const LcdBus &bus, uint8_t data) noexcept { bus.Data(data); }

/**
 * @brief バッファデータ送信ヘルパ
 * @param bus LcdBus
 * @param data 送信元バッファ
 * @param byte_size バイト数
 */
void St7789Panel::DAT(const LcdBus &bus, const uint8_t *data, size_t byte_size) noexcept { bus.Data(data, byte_size); }

/**
 * @brief ST7789のリセットシーケンスを実行する
 *
 * @param bus LcdBus
 */
void St7789Panel::Reset(const LcdBus &bus) noexcept
{
    bus.ResetDeassert();
    usleep(5 * 1000);
    bus.ResetAssert();
    usleep(20 * 1000);
    bus.ResetDeassert();
    usleep(5 * 1000);
}

/**
 * @brief 画面全域をアドレスウィンドウに設定する
 *
 * @param bus LcdBus
 */
void St7789Panel::SetFullAddr(const LcdBus &bus) noexcept
{
    // CASET X = 0..239
    CMD(bus, CMD_CASET);
    uint8_t col[] = {0x00, 0x00, 0x00, 0xEF};
    DAT(bus, col, sizeof(col));

    // RASET Y = 0..319
    CMD(bus, CMD_RASET);
    uint8_t row[] = {0x00, 0x00, 0x01, 0x3F};
    DAT(bus, row, sizeof(row));
}

/**
 * @brief ST7789の初期化を行う
 *
 * - Reset
 * - SWRESET
 * - RGB565(16bpp) 設定
 * - 反転 ON（INVON）
 * - MADCTL 初期値（ここでは 0x00、SetRotation で上書き想定）
 * - 画面全域のアドレス設定
 * - Sleep out / Display on
 *
 * @param bus LcdBus
 */
void St7789Panel::Init(const LcdBus &bus) noexcept
{
    Reset(bus);
    CMD(bus, CMD_SWRESET);
    usleep(120 * 1000);

    // 16bpp
    CMD(bus, CMD_COLMOD);
    DAT(bus, COLMOD_16BPP);

    // ディスプレイ反転を有効にする
    CMD(bus, CMD_INVON);

    // MADCTLは後でSetRotationで上書き（初期はBGRのみ）
    CMD(bus, CMD_MADCTL);
    DAT(bus, 0x00);

    SetFullAddr(bus);

    CMD(bus, CMD_SLP_OUT);
    usleep(120 * 1000);
    CMD(bus, CMD_DISP_ON);
    usleep(20 * 1000);
}

/**
 * @brief 回転（MADCTL）を設定する
 *
 * @param bus    LcdBus
 * @param madctl MADCTL に書き込む値
 */
void St7789Panel::SetRotation(const LcdBus &bus, uint8_t madctl) noexcept
{
    CMD(bus, CMD_MADCTL);
    DAT(bus, madctl);
    SetFullAddr(bus);
}

/**
 * @brief 描画領域を設定し、RAMWRを発行する
 *
 * @param bus  LcdBus
 * @param x0 左上X
 * @param y0 左上Y
 * @param x1 右下X
 * @param y1 右下Y
 */
void St7789Panel::SetAddrWindow(const LcdBus &bus, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) noexcept
{
    // 範囲逆転が来た場合は入れ替える
    if (x0 > x1) {
        std::swap(x0, x1);
    }
    if (y0 > y1) {
        std::swap(y0, y1);
    }

    uint8_t col[] = {uint8_t(x0 >> 8), uint8_t(x0), uint8_t(x1 >> 8), uint8_t(x1)};
    uint8_t row[] = {uint8_t(y0 >> 8), uint8_t(y0), uint8_t(y1 >> 8), uint8_t(y1)};

    CMD(bus, CMD_CASET);
    DAT(bus, col, 4);
    CMD(bus, CMD_RASET);
    DAT(bus, row, 4);
    CMD(bus, CMD_RAMWR);
}

/**
 * @brief 画面の簡易テストパターンを表示する
 *
 * @param bus LcdBus
 * @param bgr true の場合 MADCTL_BGR を立てる
 * @return 常に true（将来的に通信エラー等を検出する場合は拡張）
 */
bool St7789Panel::SelfTestPattern(LcdBus &bus, bool bgr) noexcept
{
    // 回転なし + RGB/BGR 選択のみ
    {
        uint8_t mad = 0x00;
        if (bgr) {
            mad |= MADCTL_BGR;
        }

        CMD(bus, CMD_MADCTL);
        DAT(bus, mad);
    }

    // 画面全域
    SetFullAddr(bus);

    CMD(bus, CMD_RAMWR);

    // ベタ塗り（赤→緑→青）
    auto fill = [&](uint16_t rgb565) {
        constexpr uint32_t kTotalPx = PHYS_W * PHYS_H;
        constexpr uint16_t kChunkPx = 512;
        alignas(4) uint8_t buf[kChunkPx * 2];

        // ST7789はMSBファースト → MSB→LSB で詰める
        for (uint16_t i = 0; i < kChunkPx; ++i) {
            buf[2 * i + 0] = static_cast<uint8_t>(rgb565 >> 8);    // MSB
            buf[2 * i + 1] = static_cast<uint8_t>(rgb565 & 0xFF);  // LSB
        }

        uint32_t left = kTotalPx;
        while (left > 0) {
            const uint16_t n = (left > kChunkPx) ? kChunkPx : static_cast<uint16_t>(left);
            DAT(bus, buf, static_cast<size_t>(n * 2u));
            left -= n;
        }
        usleep(100 * 1000);
    };

    fill(0xF800);  // Red
    usleep(10000);
    fill(0x07E0);  // Green
    usleep(10000);
    fill(0x001F);  // Blue

    return true;
}

}  // namespace platform
}  // namespace core1
