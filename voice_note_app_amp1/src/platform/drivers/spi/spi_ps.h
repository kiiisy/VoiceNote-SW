/**
 * @file spi_ps.h
 * @brief PSのSPIラッパクラス定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xspips.h"

namespace core1 {
namespace platform {

/**
 * @brief PSのSPIドライバクラス
 */
class SpiPs
{
public:
    /**
     * @brief SpiPs 設定
     */
    struct Config
    {
        uint32_t base_addr;      // ベースアドレス
        uint8_t  clk_prescaler;  // クロック分周値
        uint8_t  slave_select;   // スレーブ選択値
        uint32_t options;        // オプション
    };

    explicit SpiPs(const Config &c) : cfg_(c) {}

    int32_t Init();
    int32_t Transfer(const uint8_t *, uint8_t *, uint32_t);

    /**
     * @brief 転送完了待ち（ビジー解除待ち）
     */
    bool WaitTxDone(uint32_t loop_max = 1'000'000U) noexcept
    {
        while (spi_.IsBusy) {
            if (loop_max-- == 0U) {
                return false;
            }
        }
        while (spi_.RemainingBytes != 0U) {
            if (loop_max-- == 0U) {
                return false;
            }
        }
        __asm__ volatile("" ::: "memory");
        return true;
    }

    int32_t BeginStream() noexcept { return XSpiPs_SetSlaveSelect(&spi_, cfg_.slave_select); }
    int32_t EndStream() noexcept { return XSpiPs_SetSlaveSelect(&spi_, 0xF); }

private:
    XSpiPs spi_{};  // SPIドライバの実体
    Config cfg_{};  // 設定
};

}  // namespace platform
}  // namespace core1
