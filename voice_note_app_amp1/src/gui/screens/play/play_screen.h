/**
 * @file play_screen.h
 * @brief PLAY画面のUI生成
 *
 * CreatePlayUi()によりPLAY画面UIを生成し、ユーザー操作（Menu/Main/Back）を
 * コールバックでアプリ層へ通知する。
 */
#pragma once

// LVGLライブラリ
#include "lvgl.h"

#include "playlist_screen.h"

namespace core1 {
namespace gui {

using play_nav_cb_t = void (*)(void *user);

enum class play_view_state_t : uint8_t
{
    ShowPlay,
    ShowPause
};

struct play_ui_t
{
    lv_obj_t *root;  // ルート（親）オブジェクト

    lv_obj_t *label_title;  // 右上 "PLAY" ラベル
    lv_obj_t *btn_menu;     // 左上メニューボタン
    lv_obj_t *label_menu;   // メニューアイコン

    lv_obj_t *btn_main;    // 中央のメインボタン
    lv_obj_t *label_main;  // メインボタン内の表示（▶/⏸）

    lv_obj_t *label_file;  // file_name.wav
    lv_obj_t *slider;      // 下部スライダー
    lv_obj_t *label_pos;   // 00:00（左下）
    lv_obj_t *label_rem;   // -03:12（右下）

    playlsit_ui_t file_sheet;

    play_nav_cb_t on_menu{nullptr};
    play_nav_cb_t on_back{nullptr};
    play_nav_cb_t on_main{nullptr};
    void         *cb_user{nullptr};
};

typedef void (*nav_cb_t)(void *user);

void CreatePlayUi(lv_obj_t *parent, play_ui_t *ui);
void SetPlayMainCallback(play_ui_t *ui, nav_cb_t on_main, void *user);
void SetPlayBackCallback(play_ui_t *ui, nav_cb_t on_back, void *user);
void SetPlayMenuCallback(play_ui_t *ui, nav_cb_t on_menu, void *user);
void SetPlayViewState(play_ui_t *ui, play_view_state_t st);
void SetPlaySeek(play_ui_t *ui, const char *filename, uint32_t position_ms, uint32_t duration_ms);

}  // namespace gui
}  // namespace core1
