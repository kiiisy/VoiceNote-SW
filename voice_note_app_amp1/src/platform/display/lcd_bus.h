/**
 * @file lcd_bus.h
 * @brief LCDパネル制御のSPIとGPIOバス抽象クラス
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "gpio_ps.h"
#include "spi_ps.h"

namespace core1 {
namespace platform {

class SpiPs;
class GpioPs;

/**
 * @brief 強制inline指定
 *
 * SPI / GPIO の細粒度アクセスを関数呼び出しオーバーヘッドなしで行うため。
 */
#if defined(__GNUC__)
#define LCD_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define LCD_ALWAYS_INLINE inline
#endif

/**
 * @brief LCD用 SPIバス制御クラス
 */
class LcdBus
{
public:
    /**
     * @brief リセット信号の極性
     */
    enum class ResetPolarity : uint8_t
    {
        ActiveLow,  // Lowレベルでリセット
        ActiveHigh  // Highレベルでリセット
    };

    LcdBus(SpiPs &spi, GpioPs &gpio, uint32_t dc, uint32_t rst, ResetPolarity pol = ResetPolarity::ActiveLow)
        : spi_(spi), gpio_(gpio), dc_(dc), rst_(rst), pol_(pol)
    {
    }

    /**
     * @brief LCDコマンドを送信する
     *
     * DC=0 に設定し、1バイトのコマンドをSPI送信する。
     *
     * @param cmd コマンド値
     */
    LCD_ALWAYS_INLINE void Cmd(uint8_t cmd) const noexcept
    {
        gpio_.WritePin(dc_, 0);
        spi_.Transfer(&cmd, nullptr, 1);
    }

    /**
     * @brief LCDデータを送信する（バッファ）
     *
     * DC=1 に設定し、指定サイズのデータをSPI送信する。
     *
     * @param data データバッファ
     * @param byte_size バイト数
     */
    LCD_ALWAYS_INLINE void Data(const uint8_t *data, uint32_t byte_size) const noexcept
    {
        gpio_.WritePin(dc_, 1);
        spi_.Transfer(data, nullptr, byte_size);
    }

    /**
     * @brief LCDデータを送信する（1バイト）
     *
     * @param data データ値
     */
    LCD_ALWAYS_INLINE void Data(uint8_t data) const noexcept { Data(&data, 1); }

    /**
     * @brief LCDリセットをアサートする
     *
     * ResetPolarityに従ってRSTピンをアクティブにする。
     */
    LCD_ALWAYS_INLINE void ResetAssert() const noexcept
    {
        const uint32_t level = (pol_ == ResetPolarity::ActiveLow) ? 0u : 1u;
        gpio_.WritePin(rst_, level);
    }

    /**
     * @brief LCDリセットを解除する
     */
    LCD_ALWAYS_INLINE void ResetDeassert() const noexcept
    {
        const uint32_t level = (pol_ == ResetPolarity::ActiveLow) ? 1u : 0u;
        gpio_.WritePin(rst_, level);
    }

    /**
     * @brief SPI送信完了を待つ
     */
    LCD_ALWAYS_INLINE void WaitTxDone() const noexcept { (void)spi_.WaitTxDone(); }

    /**
     * @brief SPIストリーム転送開始
     *
     * 連続した Data() 呼び出し前に使用する。
     */
    LCD_ALWAYS_INLINE void BeginStream() const noexcept { (void)spi_.BeginStream(); }

    /**
     * @brief SPIストリーム転送終了
     */
    LCD_ALWAYS_INLINE void EndStream() const noexcept { (void)spi_.EndStream(); }

private:
    SpiPs        &spi_;   // SPIドライバ参照
    GpioPs       &gpio_;  // GPIO ドライバ参照
    uint32_t      dc_;    // DCピン番号
    uint32_t      rst_;   // RSTピン番号
    ResetPolarity pol_;   // RST極性
};

}  // namespace platform
}  // namespace core1
