/**
 * @file play_file_sheet_screen.h
 * @brief PLAY画面内 Bottom Sheet（ファイル選択UI）
 *
 * - ドラッグで開閉
 * - 5件固定表示
 * - 再生中ハイライト（左バー＋▶）
 * - 背景暗転制御
 *
 * PlayScreenの内部コンポーネントとして使用する。
 */
#pragma once

#include "lvgl.h"
#include <cstdint>

namespace core1 {
namespace gui {

using file_select_cb_t = void (*)(int16_t index, void *user);

struct playlsit_ui_t
{
    lv_obj_t *root{nullptr};     // PlayScreen root
    lv_obj_t *overlay{nullptr};  // 暗転用
    lv_obj_t *sheet{nullptr};    // Bottom sheet
    lv_obj_t *handle{nullptr};   // 上部ハンドル
    lv_obj_t *list{nullptr};     // スクロールする項目リスト

    static constexpr uint16_t kMaxItems = 10;

    lv_obj_t *items[kMaxItems]{};
    lv_obj_t *labels[kMaxItems]{};
    lv_obj_t *icons[kMaxItems]{};
    lv_obj_t *bars[kMaxItems]{};

    // drag state
    bool       dragging{false};
    lv_coord_t drag_start_y{0};
    lv_coord_t sheet_start_y{0};
    lv_coord_t sheet_closed_y{0};

    lv_obj_t *hit{nullptr};  // 画面下のドラッグ受付

    // item tap guard
    lv_point_t item_press_point{0, 0};
    bool       item_press_valid{false};
    bool       list_scrolling{false};
    uint32_t   last_scroll_tick{0};

    // callback
    file_select_cb_t on_select{nullptr};
    void            *cb_user{nullptr};

    int16_t current_playing{-1};  // 再生中インデックス
};

/**
 * @brief Bottom Sheetを生成する
 *
 * @param parent PlayScreenのroot
 * @param[out] ui 生成したUI構造体
 */
void CreatePlayListUi(lv_obj_t *parent, playlsit_ui_t *ui);

/**
 * @brief ファイル名をセットする（5件分）
 */
void SetPlayListItems(playlsit_ui_t *ui, const char *names[playlsit_ui_t::kMaxItems]);

/**
 * @brief 再生中インデックスを更新
 */
void SetPlayListPlaying(playlsit_ui_t *ui, int16_t index);

/**
 * @brief 選択コールバック登録
 */
void SetPlayListSelectCallback(playlsit_ui_t *ui, file_select_cb_t cb, void *user);

}  // namespace gui
}  // namespace core1
