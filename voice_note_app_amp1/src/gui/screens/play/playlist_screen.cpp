#include "playlist_screen.h"
#include "display/display_spec.h"
#include "logger_core.h"
#include "screen_utility.h"
#include <cstring>

namespace core1 {
namespace gui {

namespace {

static constexpr lv_coord_t kSheetOpenY     = 70;
static constexpr uint32_t   kAnimTimeMs     = 250;
static constexpr uint8_t    kMaxOverlayOpa  = 15;
static constexpr bool       kLogScrollDebug = false;
static constexpr lv_coord_t kTapMoveThresh  = 4;
static constexpr uint32_t   kTapBlockMs     = 300;
static constexpr lv_coord_t kDragHitHeight  = 96;
static constexpr lv_coord_t kListItemHeight = 34;
static constexpr lv_coord_t kListItemGap    = 4;
static constexpr lv_coord_t kListBottomPad  = 16;

// --------------------------------------------------------
// Utility
// --------------------------------------------------------
void EnableScrollBubble(lv_obj_t *obj)
{
    if (!obj) {
        return;
    }
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
}

void LogObjEvent(const char *tag, lv_event_t *e, playlsit_ui_t *ui)
{
    if (!kLogScrollDebug || !e || !ui) {
        return;
    }

    lv_event_code_t code       = lv_event_get_code(e);
    lv_obj_t       *tgt        = static_cast<lv_obj_t *>(lv_event_get_target(e));
    lv_obj_t       *cur        = static_cast<lv_obj_t *>(lv_event_get_current_target(e));
    lv_indev_t     *indev      = lv_event_get_indev(e);
    lv_obj_t       *scroll_obj = indev ? lv_indev_get_scroll_obj(indev) : nullptr;

    LOGI("[%s] code=%s tgt=%p cur=%p scroll_obj=%p list=%p top=%ld bottom=%ld y=%ld", tag, lv_event_code_get_name(code),
         (void *)tgt, (void *)cur, (void *)scroll_obj, (void *)ui->list, (long)lv_obj_get_scroll_top(ui->list),
         (long)lv_obj_get_scroll_bottom(ui->list), (long)lv_obj_get_scroll_y(ui->list));
}

void OnListDebugEvent(lv_event_t *e)
{
    auto *ui = static_cast<playlsit_ui_t *>(lv_event_get_user_data(e));
    LogObjEvent("list", e, ui);
}

lv_coord_t AbsCoord(lv_coord_t v) { return (v < 0) ? -v : v; }

int16_t FindItemIndexFromTarget(playlsit_ui_t *ui, lv_obj_t *obj)
{
    if (!ui || !obj) {
        return -1;
    }

    lv_obj_t *node = obj;
    while (node) {
        for (uint16_t i = 0; i < playlsit_ui_t::kMaxItems; ++i) {
            if (ui->items[i] == node) {
                return static_cast<int16_t>(i);
            }
        }
        node = lv_obj_get_parent(node);
    }

    return -1;
}

void OnListScrollState(lv_event_t *e)
{
    auto *ui = static_cast<playlsit_ui_t *>(lv_event_get_user_data(e));
    if (!ui) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SCROLL_END) {
        ui->list_scrolling = false;
    } else {
        ui->list_scrolling = true;
    }
    ui->last_scroll_tick = lv_tick_get();
}

void SetOverlayOpa(playlsit_ui_t *ui, lv_opa_t opa)
{
    if (!ui || !ui->overlay) {
        return;
    }
    lv_obj_set_style_bg_opa(ui->overlay, opa, 0);
}

void AnimateSheetTo(playlsit_ui_t *ui, lv_coord_t target_y)
{
    if (!ui || !ui->sheet) {
        return;
    }

    lv_coord_t start = lv_obj_get_y(ui->sheet);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui->sheet);
    lv_anim_set_exec_cb(&a, core1::gui::SetAnimY);
    lv_anim_set_values(&a, start, target_y);
    lv_anim_set_time(&a, kAnimTimeMs);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    // overlayも同期
    lv_anim_t ao;
    lv_anim_init(&ao);
    lv_anim_set_var(&ao, ui);
    lv_anim_set_exec_cb(&ao, [](void *var, int32_t v) {
        auto *u = static_cast<playlsit_ui_t *>(var);
        SetOverlayOpa(u, (lv_opa_t)v);
    });
    lv_anim_set_values(&ao, lv_obj_get_style_bg_opa(ui->overlay, 0), (target_y == kSheetOpenY) ? kMaxOverlayOpa : 0);
    lv_anim_set_time(&ao, kAnimTimeMs);
    lv_anim_set_path_cb(&ao, lv_anim_path_ease_out);
    lv_anim_start(&ao);

    if (target_y == kSheetOpenY) {
        lv_obj_clear_flag(ui->overlay, LV_OBJ_FLAG_HIDDEN);
        // 開いている間は下端ヒット領域を無効化して、リストスクロールを優先する
        lv_obj_add_flag(ui->hit, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui->overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui->hit, LV_OBJ_FLAG_HIDDEN);
    }
}

bool GetPointFromEvent(lv_event_t *e, lv_point_t *p)
{
    lv_indev_t *indev = lv_event_get_indev(e);
    if (!indev) {
        return false;
    }
    lv_indev_get_point(indev, p);
    return true;
}

// --------------------------------------------------------
// Drag Handlers
// --------------------------------------------------------
void OnPressed(lv_event_t *e)
{
    auto *ui = static_cast<playlsit_ui_t *>(lv_event_get_user_data(e));
    if (!ui) {
        return;
    }

    lv_point_t p;
    if (!GetPointFromEvent(e, &p)) {
        return;
    }

    if (kLogScrollDebug) {
        void *tgt = lv_event_get_target(e);
        LOGI("sheet pressed! target=%p hit=%p handle=%p list=%p", tgt, (void *)ui->hit, (void *)ui->handle,
             (void *)ui->list);
    } else {
        LOGI("sheet pressed!");
    }

    ui->dragging      = true;
    ui->drag_start_y  = p.y;
    ui->sheet_start_y = lv_obj_get_y(ui->sheet);
}

void OnPressing(lv_event_t *e)
{
    auto *ui = static_cast<playlsit_ui_t *>(lv_event_get_user_data(e));
    if (!ui || !ui->dragging) {
        return;
    }

    lv_point_t p;
    if (!GetPointFromEvent(e, &p)) {
        return;
    }

    lv_coord_t delta = p.y - ui->drag_start_y;
    lv_coord_t new_y = ui->sheet_start_y + delta;

    if (new_y < kSheetOpenY) {
        new_y = kSheetOpenY;
    }
    if (new_y > ui->sheet_closed_y) {
        new_y = ui->sheet_closed_y;
    }

    lv_obj_set_y(ui->sheet, new_y);

    uint32_t ratio = (ui->sheet_closed_y - new_y) * kMaxOverlayOpa / (ui->sheet_closed_y - kSheetOpenY);
    SetOverlayOpa(ui, (lv_opa_t)ratio);

    if (new_y < ui->sheet_closed_y) {
        lv_obj_clear_flag(ui->overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->hit, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(ui->hit, LV_OBJ_FLAG_HIDDEN);
    }
}

void OnReleased(lv_event_t *e)
{
    auto *ui = static_cast<playlsit_ui_t *>(lv_event_get_user_data(e));
    if (!ui) {
        return;
    }

    ui->dragging = false;

    lv_coord_t y = lv_obj_get_y(ui->sheet);

    if (y < (kSheetOpenY + ui->sheet_closed_y) / 2) {
        AnimateSheetTo(ui, kSheetOpenY);
    } else {
        AnimateSheetTo(ui, ui->sheet_closed_y);
    }
}

// --------------------------------------------------------
// Item Tap
// --------------------------------------------------------
void OnItemPressed(lv_event_t *e)
{
    auto *ui = static_cast<playlsit_ui_t *>(lv_event_get_user_data(e));
    if (!ui) {
        return;
    }

    lv_point_t p;
    if (!GetPointFromEvent(e, &p)) {
        ui->item_press_valid = false;
        return;
    }

    ui->item_press_point = p;
    ui->item_press_valid = true;
}

void OnItemReleased(lv_event_t *e)
{
    auto *ui  = static_cast<playlsit_ui_t *>(lv_event_get_user_data(e));
    auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));

    if (!ui) {
        return;
    }

    LogObjEvent("item", e, ui);

    lv_indev_t *indev = lv_event_get_indev(e);
    if (indev && lv_indev_get_scroll_obj(indev) == ui->list) {
        return;  // スクロール操作中は選択しない
    }
    if (ui->list_scrolling || lv_tick_elaps(ui->last_scroll_tick) < kTapBlockMs) {
        return;
    }

    if (!ui->item_press_valid) {
        return;
    }

    if (indev) {
        lv_point_t p{};
        lv_indev_get_point(indev, &p);
        if (AbsCoord(p.y - ui->item_press_point.y) > kTapMoveThresh ||
            AbsCoord(p.x - ui->item_press_point.x) > kTapMoveThresh) {
            return;
        }
    }

    const int16_t index = FindItemIndexFromTarget(ui, obj);
    if ((index < 0) || (index >= static_cast<int16_t>(playlsit_ui_t::kMaxItems))) {
        return;
    }

    ui->current_playing = index;
    for (uint16_t j = 0; j < playlsit_ui_t::kMaxItems; ++j) {
        if (j == static_cast<uint16_t>(index)) {
            lv_obj_clear_flag(ui->bars[j], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui->bars[j], LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ui->on_select) {
        ui->on_select(index, ui->cb_user);
    }

    AnimateSheetTo(ui, ui->sheet_closed_y);
}

}  // namespace

void SetPlayListItems(playlsit_ui_t *ui, const char *names[playlsit_ui_t::kMaxItems])
{
    lv_coord_t visible_row = 0;

    for (uint16_t i = 0; i < playlsit_ui_t::kMaxItems; ++i) {
        const char *name   = names[i] ? names[i] : "";
        const bool  hasRow = (name[0] != '\0');

        lv_label_set_text(ui->labels[i], name);
        if (hasRow) {
            lv_obj_set_y(ui->items[i], visible_row * (kListItemHeight + kListItemGap));
            lv_obj_clear_flag(ui->items[i], LV_OBJ_FLAG_HIDDEN);
            ++visible_row;
        } else {
            // 実ファイルが無い行は丸ごと隠して、空枠/空ボタンを出さない
            lv_obj_add_flag(ui->items[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void SetPlayListPlaying(playlsit_ui_t *ui, int16_t index)
{
    ui->current_playing = index;

    for (uint16_t i = 0; i < playlsit_ui_t::kMaxItems; ++i) {
        if (i == index) {
            lv_obj_clear_flag(ui->bars[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui->bars[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void SetPlayListSelectCallback(playlsit_ui_t *ui, file_select_cb_t cb, void *user)
{
    ui->on_select = cb;
    ui->cb_user   = user;
}

void CreatePlayListUi(lv_obj_t *parent, playlsit_ui_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->root = parent;

    // ---- overlay ----
    ui->overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(ui->overlay);
    lv_obj_set_size(ui->overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(ui->overlay, core1::gui::color::Black(), 0);
    lv_obj_set_style_bg_opa(ui->overlay, LV_OPA_0, 0);
    lv_obj_add_flag(ui->overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui->overlay, LV_OBJ_FLAG_CLICKABLE);

    // ---- sheet ----
    ui->sheet = lv_obj_create(parent);
    lv_obj_set_size(ui->sheet, lv_pct(100), 190);
    ui->sheet_closed_y = lv_obj_get_content_height(parent);
    if (ui->sheet_closed_y <= kSheetOpenY) {
        ui->sheet_closed_y = common::display::kHeight;
    }
    lv_obj_set_pos(ui->sheet, 0, ui->sheet_closed_y);
    lv_obj_set_style_radius(ui->sheet, 18, 0);
    lv_obj_set_style_border_width(ui->sheet, 0, 0);
    lv_obj_set_style_bg_color(ui->sheet, core1::gui::color::PlaylistSheetBg(), 0);

    lv_obj_clear_flag(ui->sheet, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->sheet, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(ui->sheet, LV_SCROLLBAR_MODE_OFF);

    // ---- handle ----
    //ui->handle = lv_obj_create(parent);
    ui->handle = lv_obj_create(ui->sheet);
    lv_obj_set_size(ui->handle, 44, 24);
    //lv_obj_align(ui->handle, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_align(ui->handle, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_radius(ui->handle, 12, 0);
    lv_obj_set_style_bg_color(ui->handle, core1::gui::color::PlaylistSheetBg(), 0);
    lv_obj_set_style_bg_opa(ui->handle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ui->handle, 0, 0);
    lv_obj_set_style_shadow_width(ui->handle, 0, 0);

    // ---- scroll list container ----
    ui->list = lv_obj_create(ui->sheet);
    lv_obj_remove_style_all(ui->list);
    lv_obj_set_size(ui->list, lv_pct(100), 150);
    lv_obj_align(ui->list, LV_ALIGN_TOP_MID, 0, 34);
    lv_obj_set_style_pad_all(ui->list, 0, 0);
    lv_obj_set_style_pad_row(ui->list, kListItemGap, 0);
    lv_obj_set_style_pad_bottom(ui->list, kListBottomPad, 0);
    lv_obj_set_style_border_width(ui->list, 0, 0);
    lv_obj_set_style_bg_opa(ui->list, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(ui->list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui->list, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_flag(ui->list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scroll_dir(ui->list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui->list, LV_SCROLLBAR_MODE_OFF);
    EnableScrollBubble(ui->list);
    lv_obj_add_event_cb(ui->list, OnListDebugEvent, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(ui->list, OnListDebugEvent, LV_EVENT_PRESSING, ui);
    lv_obj_add_event_cb(ui->list, OnListDebugEvent, LV_EVENT_RELEASED, ui);
    lv_obj_add_event_cb(ui->list, OnListDebugEvent, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->list, OnListDebugEvent, LV_EVENT_SCROLL, ui);
    lv_obj_add_event_cb(ui->list, OnListDebugEvent, LV_EVENT_SCROLL_END, ui);
    lv_obj_add_event_cb(ui->list, OnListScrollState, LV_EVENT_SCROLL_BEGIN, ui);
    lv_obj_add_event_cb(ui->list, OnListScrollState, LV_EVENT_SCROLL, ui);
    lv_obj_add_event_cb(ui->list, OnListScrollState, LV_EVENT_SCROLL_END, ui);

    // ---- list items ----
    for (uint16_t i = 0; i < playlsit_ui_t::kMaxItems; ++i) {
        // 行コンテナ
        ui->items[i] = lv_obj_create(ui->list);
        lv_obj_remove_style_all(ui->items[i]);  // ★重要：テーマを剥がす（文字ズレ対策の本丸）
        lv_obj_set_size(ui->items[i], lv_pct(100), kListItemHeight);
        lv_obj_align(ui->items[i], LV_ALIGN_TOP_LEFT, 0, i * (kListItemHeight + kListItemGap));
        lv_obj_clear_flag(ui->items[i], LV_OBJ_FLAG_SCROLLABLE);
        EnableScrollBubble(ui->items[i]);

        // ★押下しても見た目が変わらない固定スタイル（アニメ無し）
        lv_obj_set_style_pad_all(ui->items[i], 0, 0);
        lv_obj_set_style_border_width(ui->items[i], 0, 0);
        lv_obj_set_style_outline_width(ui->items[i], 0, 0);
        lv_obj_set_style_bg_opa(ui->items[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(ui->items[i], LV_OPA_TRANSP, LV_STATE_PRESSED);
        lv_obj_set_style_transform_zoom(ui->items[i], 256, 0);
        lv_obj_set_style_transform_zoom(ui->items[i], 256, LV_STATE_PRESSED);
        lv_obj_set_style_translate_x(ui->items[i], 0, 0);
        lv_obj_set_style_translate_x(ui->items[i], 0, LV_STATE_PRESSED);
        lv_obj_set_style_translate_y(ui->items[i], 0, 0);
        lv_obj_set_style_translate_y(ui->items[i], 0, LV_STATE_PRESSED);

        // 左の再生中バー
        ui->bars[i] = lv_obj_create(ui->items[i]);
        lv_obj_remove_style_all(ui->bars[i]);
        lv_obj_set_size(ui->bars[i], 4, lv_pct(100));
        lv_obj_align(ui->bars[i], LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_color(ui->bars[i], core1::gui::color::PlaylistAccent(), 0);
        lv_obj_set_style_bg_opa(ui->bars[i], LV_OPA_COVER, 0);
        lv_obj_add_flag(ui->bars[i], LV_OBJ_FLAG_HIDDEN);

        // 右側：固定幅コンテナ
        static constexpr lv_coord_t kRightW = 60;
        lv_obj_t                   *right   = lv_obj_create(ui->items[i]);
        lv_obj_remove_style_all(right);
        lv_obj_set_size(right, kRightW, lv_pct(100));
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(right, LV_OBJ_FLAG_CLICKABLE);
        EnableScrollBubble(right);

        // 右側アイコン
        ui->icons[i] = lv_label_create(right);
        lv_label_set_text(ui->icons[i], LV_SYMBOL_PLAY);
        lv_obj_set_style_text_color(ui->icons[i], core1::gui::color::PlaylistAccent(), 0);
        lv_obj_center(ui->icons[i]);

        // ファイル名ラベル（★2行禁止：DOT + 幅固定）
        ui->labels[i] = lv_label_create(ui->items[i]);
        lv_label_set_text(ui->labels[i], "");
        lv_obj_set_style_text_color(ui->labels[i], core1::gui::color::White(), 0);
        lv_label_set_long_mode(ui->labels[i], LV_LABEL_LONG_DOT);  // ★2行禁止の要
        lv_obj_align(ui->labels[i], LV_ALIGN_LEFT_MID, 12, 0);

        static constexpr lv_coord_t kScreenW = static_cast<lv_coord_t>(common::display::kWidth);
        lv_obj_set_width(ui->labels[i], kScreenW - 12 - 4 - 6 - kRightW - 6);

        // 子を触ってもスクロール/イベントを親へ伝播
        EnableScrollBubble(ui->labels[i]);
        EnableScrollBubble(ui->bars[i]);
        EnableScrollBubble(ui->icons[i]);

        // 行全体はスクロール優先。再生は右端アイコン領域だけで受ける。
        lv_obj_add_event_cb(right, OnItemPressed, LV_EVENT_PRESSED, ui);
        lv_obj_add_event_cb(right, OnItemReleased, LV_EVENT_CLICKED, ui);
    }

    if (kLogScrollDebug) {
        LOGI("[list_init] list=%p scroll_top=%ld scroll_bottom=%ld", (void *)ui->list,
             (long)lv_obj_get_scroll_top(ui->list), (long)lv_obj_get_scroll_bottom(ui->list));
    }

    // ---- bottom hit area (drag entry) ----
    ui->hit = lv_obj_create(parent);
    lv_obj_remove_style_all(ui->hit);
    lv_obj_set_size(ui->hit, lv_pct(100), kDragHitHeight);  // 下部のドラッグ受付を広めに取る
    lv_obj_align(ui->hit, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(ui->hit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui->hit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui->hit, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(ui->hit, LV_SCROLLBAR_MODE_OFF);

    lv_obj_add_event_cb(ui->hit, OnPressed, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(ui->hit, OnPressing, LV_EVENT_PRESSING, ui);
    lv_obj_add_event_cb(ui->hit, OnReleased, LV_EVENT_RELEASED, ui);

    lv_obj_add_event_cb(ui->handle, OnPressed, LV_EVENT_PRESSED, ui);
    lv_obj_add_event_cb(ui->handle, OnPressing, LV_EVENT_PRESSING, ui);
    lv_obj_add_event_cb(ui->handle, OnReleased, LV_EVENT_RELEASED, ui);

    lv_obj_move_foreground(ui->hit);
}

}  // namespace gui
}  // namespace core1
