/**
 * @file screen_store.cpp
 * @brief screen_storeの実装
 */

// 自ヘッダー
#include "screen_store.h"

namespace core1 {
namespace gui {

void ScreenStore::Init()
{
    // スクリーンを生成
    home_screen_     = lv_obj_create(nullptr);
    rec_screen_      = lv_obj_create(nullptr);
    play_screen_     = lv_obj_create(nullptr);
    play_agc_screen_ = lv_obj_create(nullptr);
    rec_option_screen_ = lv_obj_create(nullptr);

    // サイズ設定
    lv_display_t *d = lv_display_get_default();
    auto          w = lv_display_get_horizontal_resolution(d);
    auto          h = lv_display_get_vertical_resolution(d);

    lv_obj_set_size(home_screen_, w, h);
    lv_obj_set_size(rec_screen_, w, h);
    lv_obj_set_size(play_screen_, w, h);
    lv_obj_set_size(play_agc_screen_, w, h);
    lv_obj_set_size(rec_option_screen_, w, h);

    // UI生成
    CreateHomeUi(home_screen_, &home_ui_);
    CreateRecordUi(rec_screen_, &rec_ui_);
    CreatePlayUi(play_screen_, &play_ui_);
    CreatePlayAgcUi(play_agc_screen_, &play_agc_ui_);
    CreateRecOptionUi(rec_option_screen_, &rec_option_ui_);
}

}  // namespace gui
}  // namespace core1
