/**
 * @file screen_store.h
 * @brief LVGLスクリーンと各画面UIハンドルを生成して所有するストアクラス
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// LVGLライブラリ
#include "lvgl.h"

// プロジェクトライブラリ
#include "home_screen.h"
#include "play_agc_screen.h"
#include "play_screen.h"
#include "record_screen.h"
#include "record_option_screen.h"

namespace core1 {
namespace gui {

class ScreenStore
{
public:
    ScreenStore()                               = default;
    ScreenStore(const ScreenStore &)            = delete;
    ScreenStore &operator=(const ScreenStore &) = delete;
    ScreenStore(ScreenStore &&)                 = delete;
    ScreenStore &operator=(ScreenStore &&)      = delete;

    void Init();

    // スクリーンGetter
    lv_obj_t *GetHomeScreen() const { return home_screen_; }
    lv_obj_t *GetRecScreen() const { return rec_screen_; }
    lv_obj_t *GetPlayScreen() const { return play_screen_; }
    lv_obj_t *GetPlayOptScreen() const { return play_agc_screen_; }
    lv_obj_t *GetRecOptScreen() const { return rec_option_screen_; }

    // UI Getter
    home_ui_t           &GetHomeUi() { return home_ui_; }
    const home_ui_t     &GetHomeUi() const { return home_ui_; };
    record_ui_t         &GetRecUi() { return rec_ui_; }
    const record_ui_t   &GetRecUi() const { return rec_ui_; }
    play_ui_t           &GetPlayUi() { return play_ui_; }
    const play_ui_t     &GetPlayUi() const { return play_ui_; }
    play_agc_ui_t       &GetPlayAgcUi() { return play_agc_ui_; }
    const play_agc_ui_t &GetPlayAgcUi() const { return play_agc_ui_; }
    rec_option_ui_t       &GetRecOptionUi() { return rec_option_ui_; }
    const rec_option_ui_t &GetRecOptionUi() const { return rec_option_ui_; }

private:
    lv_obj_t *home_screen_{nullptr};
    lv_obj_t *rec_screen_{nullptr};
    lv_obj_t *play_screen_{nullptr};
    lv_obj_t *play_agc_screen_{nullptr};
    lv_obj_t *rec_option_screen_{nullptr};

    home_ui_t     home_ui_{};
    record_ui_t   rec_ui_{};
    play_ui_t     play_ui_{};
    play_agc_ui_t play_agc_ui_{};
    rec_option_ui_t rec_option_ui_{};
};

}  // namespace gui
}  // namespace core1
