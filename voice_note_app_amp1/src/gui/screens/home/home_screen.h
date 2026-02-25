/**
 * @file home_screen.h
 * @brief ホーム画面（REC / PLAY）のUI定義
 *
 * CreateHomeUi()でホーム画面を生成し、
 * SetHomeUiCallback()でREC/PLAY押下をアプリ層へ通知する。
 */
#pragma once

// LVGLライブラリ
#include "lvgl.h"

namespace core1 {
namespace gui {

using home_nav_cb_t = void (*)(void *user);

/**
 * @brief ホーム画面のUIハンドル群
 *
 * CreateHomeUi()がlv_obj_tを生成して本構造体へ格納する。
 */
struct home_ui_t
{
    lv_obj_t *root;
    lv_obj_t *btn_rec;
    lv_obj_t *btn_play;
    lv_obj_t *label_rec;
    lv_obj_t *label_play;
    lv_obj_t *btn_rec_text;
    lv_obj_t *btn_play_text;

    home_nav_cb_t on_rec{nullptr};
    home_nav_cb_t on_play{nullptr};
    void         *cb_user{nullptr};
};

typedef void (*home_nav_cb_t)(void *user);

void CreateHomeUi(lv_obj_t *parent, home_ui_t *ui);
void SetHomeUiCallback(home_ui_t *ui, home_nav_cb_t on_rec, home_nav_cb_t on_play, void *user);
}  // namespace gui
}  // namespace core1
