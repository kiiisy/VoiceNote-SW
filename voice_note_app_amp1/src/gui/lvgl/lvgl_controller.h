/**
 * @file lvgl_controller.h
 * @brief LVGL制御クラス（Display/Flush/Touch/ScreenStore/UiNavigator連携）
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// LVGLライブラリ
#include "lv_example_widgets.h"
#include "lv_examples.h"
#include "lvgl.h"

// プロジェクトライブラリ
#include "gpio_ps.h"
#include "play.h"
#include "playlist.h"
#include "screen_store.h"
#include "ui_navigator.h"

namespace core1 {

namespace platform {
class LcdBus;
class PanelInterface;
class TouchCtrl;
}  // namespace platform

namespace gui {

class LvglController
{
public:
    struct Config
    {
        uint16_t hor = 320;
        uint16_t ver = 240;

        bool     swapRgb565Bytes  = true;
        uint32_t flushChunkPixels = 512;
    };

    explicit LvglController(const Config &cfg);

    void Init(platform::LcdBus &bus, platform::PanelInterface &panel);
    void RegisterTouch(platform::TouchCtrl &touch);
    void InitScreens();

    void TimerHandler() { lv_timer_handler(); }
    void TiclInc(uint32_t tm) { lv_tick_inc(tm); }

    void          SetRotation(lv_display_rotation_t rot) { lv_display_set_rotation(disp_, rot); }
    lv_display_t *handle() const { return disp_; }

    // UI → System 通知
    void SetPlayAgcCallback(UiNavigator::PlayAgcRequesteFn fn, void *user);
    void SetRecOptionCallback(UiNavigator::RecOptionRequesteFn fn, void *user);
    void SetPlayCallback(UiNavigator::PlayRequesteFn fn, void *user);
    void SetPlayListRequesteCallback(UiNavigator::PlayListRequesteFn fn, void *user);

    // System / IPC → UI
    void SetPlayView(bool playing);
    void SetPlaybackProgress(uint32_t position_ms, uint32_t duration_ms);
    void SetRecordView(bool recording);
    void SetRecordProgress(uint32_t captured_ms, uint32_t target_ms);
    void SetPlaybackUiState(uint8_t state);
    void SetRecordUiState(uint8_t state);
    void SetPlayFileList(const char *names[UiNavigator::kMaxFiles], uint16_t count);

    void SetRecRequesteCallback(UiNavigator::RecRequesteFn fn, void *user);

private:
    static void FlushDisplayCallback(lv_display_t *disp, const lv_area_t *area, uint8_t *pix);
    void        FlushDisplay(lv_display_t *disp, const lv_area_t *a, uint8_t *pix);

    static void ReadTouchCallback(lv_indev_t *indev, lv_indev_data_t *data);
    void        ReadTouch(lv_indev_t *indev, lv_indev_data_t *data);

    Config                    cfg_{};
    platform::LcdBus         *bus_{nullptr};
    platform::PanelInterface *panel_{nullptr};
    lv_display_t             *disp_{nullptr};

    lv_indev_t          *indev_{nullptr};
    platform::TouchCtrl *touch_{nullptr};

    ScreenStore store_;
    UiNavigator nav_{store_};

    // 描画バッファ
    static constexpr size_t kBufBytesMax = 240u * 320u * 2u;
    alignas(4) uint8_t drawbuf_[kBufBytesMax];

    static constexpr uint32_t kScratchBytes = 1024 * 2;
    alignas(4) uint8_t scratch_[kScratchBytes];
};

}  // namespace gui
}  // namespace core1
