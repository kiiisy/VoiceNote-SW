// 自ヘッダー
#include "ili9341_panel.h"

// Vitisライブラリ
#include "sleep.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core1 {
namespace platform {

void Ili9341Panel::Reset(const LcdBus &b)
{
    b.ResetDeassert();
    usleep(5 * 1000);
    b.ResetAssert();
    usleep(20 * 1000);
    b.ResetDeassert();
    usleep(150 * 1000);

    LOGI("ILI9341 reset done.");
}

void Ili9341Panel::Init(const LcdBus &b)
{
    auto CMD = [&](uint8_t v) { b.Cmd(v); };
    auto DAT = [&](uint8_t v) { b.Data(v); };

    CMD(0x01);
    usleep(150);

    CMD(0xCB);
    DAT(0x39);
    DAT(0x2C);
    DAT(0x00);
    DAT(0x34);
    DAT(0x02);
    CMD(0xCF);
    DAT(0x00);
    DAT(0xC1);
    DAT(0x30);
    CMD(0xE8);
    DAT(0x85);
    DAT(0x00);
    DAT(0x78);
    CMD(0xEA);
    DAT(0x00);
    DAT(0x00);
    CMD(0xED);
    DAT(0x64);
    DAT(0x03);
    DAT(0x12);
    DAT(0x81);
    CMD(0xF7);
    DAT(0x20);
    CMD(0xC0);
    DAT(0x23);
    CMD(0xC1);
    DAT(0x10);
    CMD(0xC5);
    DAT(0x3E);
    DAT(0x28);
    CMD(0xC7);
    DAT(0x86);
    CMD(0x36);
    DAT(0x80);  // MADCTL（必要に応じて変更）
    CMD(0x3A);
    DAT(0x55);  // 16bpp RGB565
    CMD(0xB1);
    DAT(0x00);
    DAT(0x18);
    CMD(0xB6);
    DAT(0x08);
    DAT(0x82);
    DAT(0x27);
    CMD(0xF2);
    DAT(0x00);
    CMD(0x26);
    DAT(0x01);

    CMD(0xE0);  // Positive gamma
    const uint8_t pg[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    b.Data(pg, sizeof(pg));
    CMD(0xE1);  // Negative gamma
    const uint8_t ng[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    b.Data(ng, sizeof(ng));

    CMD(0x11);
    usleep(150000);  // Sleep out
    CMD(0x29);
    usleep(50000);  // Display ON

    LOGI("ILI9341 init done.");
}

void Ili9341Panel::SetRotation(const LcdBus &b, uint8_t madctl)
{
    b.Cmd(0x36);
    //b.Data(madctl);
    b.Data(static_cast<uint8_t>(madctl | MADCTL_BGR));
}
}  // namespace platform
}  // namespace core1
