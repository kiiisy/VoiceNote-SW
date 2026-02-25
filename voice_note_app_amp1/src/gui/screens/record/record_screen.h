/**
 * @file record_screen.h
 * @brief 録音画面（REC）のUI定義
 *
 * CreateRecordUi()により録音画面UIを生成し、
 * SetRecordMenuCallback() / SetRecordBackCallback()により
 * Menu/Back操作をアプリ層へ通知する。
 */
#pragma once

// LVGLライブラリ
#include "lvgl.h"

namespace core1 {
namespace gui {

using record_nav_cb_t = void (*)(void *user);

/**
 * @brief 録音画面の表示状態
 */
enum class record_view_state_t : uint8_t
{
    ShowStart,  // ▶（録音開始）
    ShowStop,   // ■（録音停止）
};

/**
 * @brief 録音画面の UI ハンドル群
 *
 * CreateRecordUi()がlv_obj_tを生成して本構造体へ格納する。
 */
struct record_ui_t
{
    lv_obj_t *root;  // ルート（親）オブジェクト

    lv_obj_t *label_title;  // 左上 "REC" ラベル
    lv_obj_t *btn_menu;     // 右上メニューボタン
    lv_obj_t *label_menu;   // メニュー表示（"≡"）

    lv_obj_t *btn_main;    // 中央のメインボタン
    lv_obj_t *label_main;  // メインボタン内の表示（▶/■）
    lv_obj_t *slider;      // 下部スライダー
    lv_obj_t *label_pos;   // 録音経過
    lv_obj_t *label_rem;   // 残り時間

    record_nav_cb_t on_menu{nullptr};
    record_nav_cb_t on_back{nullptr};
    record_nav_cb_t on_main{nullptr};
    void           *cb_user{nullptr};
};

typedef void (*nav_cb_t)(void *user);

void CreateRecordUi(lv_obj_t *parent, record_ui_t *ui);
void SetRecordMenuCallback(record_ui_t *ui, nav_cb_t on_menu, void *user);
void SetRecordBackCallback(record_ui_t *ui, nav_cb_t on_back, void *user);
void SetRecordMainCallback(record_ui_t *ui, nav_cb_t on_main, void *user);
void SetRecordViewState(record_ui_t *ui, record_view_state_t st);
void SetRecordSeek(record_ui_t *ui, uint32_t captured_ms, uint32_t target_ms);

}  // namespace gui
}  // namespace core1
