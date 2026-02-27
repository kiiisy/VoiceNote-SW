/**
 * @file touch_ctrl.h
 * @brief I2Cタッチコントローラ制御クラス定義
 */
#pragma once

// 標準ライブラリ
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "display/display_spec.h"

namespace core1 {
namespace platform {

class I2cPlCore;

inline constexpr uint8_t kTouchI2cAddr7 = 0x1A;  // タッチICの7bitのI2Cアドレス

/**
 * @brief タッチ座標（画面座標系）
 */
struct TouchPoint
{
    uint16_t x{0};            // X座標
    uint16_t y{0};            // Y座標
    bool     pressed{false};  // 押下中か否か
};

/**
 * @brief タッチコントローラ制御クラス
 */
class TouchCtrl
{
public:
    /**
     * @brief 動作設定
     */
    struct Config
    {
        uint8_t addr{kTouchI2cAddr7};

        uint16_t width{common::display::kWidth};    // 画面幅
        uint16_t height{common::display::kHeight};  // 画面高

        bool swap_xy{true};    // X/Y入替
        bool invert_x{false};  // X反転
        bool invert_y{true};   // Y反転
    };

    explicit TouchCtrl(const Config &cfg) : cfg_(cfg) {}

    bool Init(I2cPlCore &i2c);
    bool Run();

    void NotifyInterrupt() noexcept { pending_.store(true, std::memory_order_release); }

    TouchPoint GetLastPoint() const noexcept { return last_point_; }
    bool       IsPressed() const noexcept { return last_point_.pressed; }

private:
    bool ReadTouchState(TouchPoint &out_point);
    void MapRawToScreen(TouchPoint &point) const;
    void MedianFilter3(TouchPoint *point);

    I2cPlCore        *i2c_{nullptr};    // I2Cの参照
    Config            cfg_{};           // 設定
    std::atomic<bool> pending_{false};  // INTでセット、Runで消費
    TouchPoint        last_point_{};    // 最後の座標
};

}  // namespace platform
}  // namespace core1
