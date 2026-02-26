/**
 * @file lvgl_controller.cpp
 * @brief lvgl_controllerの実装
 */

// 自ヘッダー
#include "lvgl_controller.h"

// 標準ライブラリ
#include <algorithm>

// プロジェクトライブラリ
#include "lcd_bus.h"
#include "logger_core.h"
#include "panel_interface.h"
#include "touch_ctrl.h"

namespace core1 {
namespace gui {

LvglController::LvglController(const Config &cfg) : cfg_(cfg) {}

/**
 * @brief LVGL初期化とDisplay生成
 */
void LvglController::Init(platform::LcdBus &bus, platform::PanelInterface &panel)
{
    LOG_SCOPE();

    bus_   = &bus;
    panel_ = &panel;

    lv_init();

    disp_ = lv_display_create(cfg_.hor, cfg_.ver);
    lv_display_set_color_format(disp_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_rotation(disp_, LV_DISPLAY_ROTATION_0);

    lv_display_set_flush_cb(disp_, &LvglController::FlushDisplayCallback);
    lv_display_set_buffers(disp_, drawbuf_, nullptr, static_cast<uint32_t>(cfg_.hor) * cfg_.ver * 2,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_user_data(disp_, this);
    lv_display_set_default(disp_);
}

/**
 * @brief ScreenStore / UiNavigator 初期化
 */
void LvglController::InitScreens()
{
    LOG_SCOPE();

    store_.Init();
    nav_.Init();
}

/**
 * @brief AGC要求コールバック
 */
void LvglController::SetPlayAgcCallback(UiNavigator::PlayAgcRequesteFn fn, void *user)
{
    nav_.SetPlayAgcRequesteCallback(fn, user);
}

void LvglController::SetRecOptionCallback(UiNavigator::RecOptionRequesteFn fn, void *user)
{
    nav_.SetRecOptionRequesteCallback(fn, user);
}

/**
 * @brief 再生要求コールバック
 */
void LvglController::SetPlayCallback(UiNavigator::PlayRequesteFn fn, void *user)
{
    nav_.SetPlayRequesteCallback(fn, user);
}

void LvglController::SetPlayListRequesteCallback(UiNavigator::PlayListRequesteFn fn, void *user)
{
    nav_.SetPlayListRequesteCallback(fn, user);
}

/**
 * @brief 再生画面コールバック
 */
void LvglController::SetPlayView(bool playing) { nav_.SetPlayView(playing); }
void LvglController::SetPlaybackProgress(uint32_t position_ms, uint32_t duration_ms)
{
    nav_.SetPlaybackProgress(position_ms, duration_ms);
}
void LvglController::SetRecordView(bool recording) { nav_.SetRecordView(recording); }
void LvglController::SetRecordProgress(uint32_t captured_ms, uint32_t target_ms)
{
    nav_.SetRecordProgress(captured_ms, target_ms);
}

void LvglController::SetPlaybackUiState(uint8_t state) { nav_.SetPlaybackUiState(state); }

void LvglController::SetRecordUiState(uint8_t state) { nav_.SetRecordUiState(state); }

void LvglController::SetPlayFileList(const char *names[UiNavigator::kMaxFiles], uint16_t count)
{
    nav_.SetPlayFileList(names, count);
}

/**
 * @brief 録音Main押下イベントの通知先を設定する
 */
void LvglController::SetRecRequesteCallback(UiNavigator::RecRequesteFn fn, void *user)
{
    nav_.SetRecRequesteCallback(fn, user);
}

/**
 * @brief LVGL Flush callback
 */
void LvglController::FlushDisplayCallback(lv_display_t *disp, const lv_area_t *area, uint8_t *pix)
{
    auto *self = static_cast<LvglController *>(lv_display_get_user_data(disp));
    if (self) {
        self->FlushDisplay(disp, area, pix);
    }
}

void LvglController::FlushDisplay(lv_display_t *, const lv_area_t *area, uint8_t *pix)
{
    panel_->SetAddrWindow(*bus_, area->x1, area->y1, area->x2, area->y2);

    const uint32_t w        = area->x2 - area->x1 + 1;
    const uint32_t h        = area->y2 - area->y1 + 1;
    const uint32_t total_px = w * h;

    const uint32_t max_chunk_px = sizeof(scratch_) / 2;
    const uint32_t chunk_px     = std::max<uint32_t>(16, std::min<uint32_t>(cfg_.flushChunkPixels, max_chunk_px));

    const uint8_t *src  = pix;
    uint32_t       sent = 0;

    while (sent < total_px) {
        const uint32_t n = std::min(chunk_px, total_px - sent);

        if (cfg_.swapRgb565Bytes) {
            uint8_t *q = scratch_;
            for (uint32_t i = 0; i < n; ++i) {
                q[0] = src[1];
                q[1] = src[0];
                q += 2;
                src += 2;
            }
            bus_->Data(scratch_, n * 2);
        } else {
            bus_->Data(src, n * 2);
            src += n * 2;
        }

        sent += n;
    }

    lv_disp_flush_ready(disp_);
}

/**
 * @brief タッチ入力登録
 */
void LvglController::RegisterTouch(platform::TouchCtrl &touch)
{
    touch_ = &touch;

    indev_ = lv_indev_create();
    lv_indev_set_type(indev_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_, &LvglController::ReadTouchCallback);
    lv_indev_set_user_data(indev_, this);
    lv_indev_set_display(indev_, disp_);
    lv_indev_enable(indev_, true);
}

void LvglController::ReadTouchCallback(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto *self = static_cast<LvglController *>(lv_indev_get_user_data(indev));
    if (self) {
        self->ReadTouch(indev, data);
    }
}

void LvglController::ReadTouch(lv_indev_t *, lv_indev_data_t *data)
{
    data->continue_reading = false;

    if (!touch_) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    // TouchCtrl側でデバウンス済みの値をそのまま使う
    auto raw = touch_->GetLastPoint();

    raw.x = std::min<uint16_t>(raw.x, cfg_.hor - 1);
    raw.y = std::min<uint16_t>(raw.y, cfg_.ver - 1);

    data->state   = raw.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = raw.x;
    data->point.y = raw.y;
}
}  // namespace gui
}  // namespace core1
