/**
 * @file play_screen.cpp
 * @brief PLAY画面の生成とイベント処理
 *
 * LVGL上にPLAY画面のUIを構築し、Main / Back / Menuの操作イベントを
 * アプリ層へコールバック通知する。
 */

// 自ヘッダー
#include "play_screen.h"

// 標準ライブラリ
#include <cstdio>
#include <string.h>

// プロジェクトライブラリ
#include "logger_core.h"
#include "screen_utility.h"

namespace core1 {
namespace gui {
namespace {

/**
 * @brief Mainボタンのクリックイベント処理
 *
 * UI上の再生表示（▶/⏸）をトグルし、アプリ層へイベント通知（g_on_main）を行う。
 *
 * @param event LVGLイベント
 */
void OnMain(lv_event_t *event)
{
    // UIの見た目トグル
    play_ui_t *ui = (play_ui_t *)lv_event_get_user_data(event);
    if (!ui || !ui->label_main) {
        return;
    }

    // アプリへ通知（イベントだけ）
    if (ui->on_main) {
        ui->on_main(ui->cb_user);
    }
}

/**
 * @brief Backボタンのイベント処理
 *
 * 画面遷移などの実処理はアプリ層で行う前提で、登録済みコールバックに通知する。
 *
 * @param event LVGLイベント
 */
void OnBack(lv_event_t *event)
{
    play_ui_t *ui = (play_ui_t *)lv_event_get_user_data(event);
    if (!ui || !ui->label_main) {
        return;
    }

    if (ui->on_back) {
        ui->on_back(ui->cb_user);
    }
}

/**
 * @brief Menuボタンのイベント処理
 *
 * メニュー画面遷移などの実処理はアプリ層で行う前提で、登録済みコールバックに通知する。
 *
 * @param event LVGLイベント
 */
void OnMenu(lv_event_t *event)
{
    play_ui_t *ui = (play_ui_t *)lv_event_get_user_data(event);
    if (!ui || !ui->label_main) {
        return;
    }

    if (ui->on_menu) {
        ui->on_menu(ui->cb_user);
    }
}

void OnFileSelected(int16_t index, void *user)
{
    auto *ui = static_cast<play_ui_t *>(user);
    if (!ui) {
        return;
    }

    if (ui->on_main) {
        // 仮：indexをtrack_idに使う
        ui->on_main(ui->cb_user);
    }
}

/**
 * @brief ルート背景（PLAY画面）スタイルを適用する
 */
void ApplyRootStylePlay(lv_obj_t *root)
{
    core1::gui::KillScroll(root);

    // 背景（Frame3：青）
    const lv_color_t BG_PLAY = LV_COLOR_RGB_AS_BGR(0x0086FF);
    lv_obj_set_style_bg_color(root, BG_PLAY, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
}

/**
 * @brief 透明なボタンの見た目を適用する
 */
void ApplyTransparentButtonStyle(lv_obj_t *btn)
{
    core1::gui::KillScroll(btn);

    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
}

/**
 * @brief 丸ボタンの見た目を適用する（背景色・不透明度など）
 */
void ApplyCircleButtonStyle(lv_obj_t *btn, lv_color_t bg, lv_opa_t opa)
{
    core1::gui::KillScroll(btn);

    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(btn, opa, 0);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
}

/**
 * @brief ボタン中央にラベル（白文字・指定フォント）を生成する
 */
lv_obj_t *CreateCenteredLabel(lv_obj_t *parent, const char *text, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_center(label);
    return label;
}

/**
 * @brief 右上PLAYボタンを生成する
 *
 * 右上のタイトルとして見せつつ、クリックでBack扱いにする。
 */
void CreateBackButton(lv_obj_t *root, play_ui_t *ui)
{
    lv_obj_t *btn_back = lv_button_create(root);
    ApplyTransparentButtonStyle(btn_back);

    lv_obj_set_size(btn_back, 130, 44);
    lv_obj_align(btn_back, LV_ALIGN_TOP_RIGHT, 8, 8);
    lv_obj_add_event_cb(btn_back, OnBack, LV_EVENT_CLICKED, ui);

    ui->label_title = CreateCenteredLabel(btn_back, "PLAY", &lv_font_montserrat_28);
}

/**
 * @brief 左上のメニューボタンを生成する
 */
void CreateMenuButton(lv_obj_t *root, play_ui_t *ui)
{
    ui->btn_menu = lv_button_create(root);

    lv_obj_set_size(ui->btn_menu, 56, 56);
    lv_obj_align(ui->btn_menu, LV_ALIGN_TOP_LEFT, 10, 10);

    // 半透明の丸ボタン（黒っぽい）
    ApplyCircleButtonStyle(ui->btn_menu, LV_COLOR_RGB_AS_BGR(0x1F1F27), LV_OPA_40);

    // 指が乗った瞬間に反応させたい
    lv_obj_add_event_cb(ui->btn_menu, OnMenu, LV_EVENT_PRESSED, ui);

    // アイコン（設定 or リスト）
#ifdef LV_SYMBOL_SETTINGS
    ui->label_menu = CreateCenteredLabel(ui->btn_menu, LV_SYMBOL_SETTINGS, &lv_font_montserrat_28);
#else
    ui->label_menu = CreateCenteredLabel(ui->btn_menu, LV_SYMBOL_LIST, &lv_font_montserrat_28);
#endif

    // 念のため最前面にする
    lv_obj_move_foreground(ui->btn_menu);
}

/**
 * @brief 中央のメインボタン（再生/停止）を生成する
 */
void CreateMainButton(lv_obj_t *root, play_ui_t *ui)
{
    ui->btn_main = lv_button_create(root);

    lv_obj_set_size(ui->btn_main, 96, 96);
    lv_obj_align(ui->btn_main, LV_ALIGN_CENTER, 0, -15);

    ApplyCircleButtonStyle(ui->btn_main, LV_COLOR_RGB_AS_BGR(0x1F1F27), LV_OPA_COVER);

    // 離したら確定にする
    lv_obj_add_event_cb(ui->btn_main, OnMain, LV_EVENT_RELEASED, ui);

    ui->label_main = CreateCenteredLabel(ui->btn_main, LV_SYMBOL_PLAY, &lv_font_montserrat_28);
}

/**
 * @brief 下部のスライダーを生成する
 */
void CreateBottomSlider(lv_obj_t *root, play_ui_t *ui)
{
    ui->slider = lv_slider_create(root);
    core1::gui::KillScroll(ui->slider);

    lv_slider_set_range(ui->slider, 0, 1000);
    lv_obj_set_width(ui->slider, lv_pct(78));
    lv_obj_set_height(ui->slider, 8);                       // 細く
    lv_obj_align(ui->slider, LV_ALIGN_BOTTOM_MID, 0, -34);  // 少し上に

    // 本体（薄め）
    lv_obj_set_style_bg_opa(ui->slider, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->slider, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_radius(ui->slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);

    // インジケータ（濃い）
    lv_obj_set_style_bg_opa(ui->slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui->slider, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(ui->slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);

    // つまみ（不要なら消す）
    lv_obj_set_style_opa(ui->slider, LV_OPA_0, LV_PART_KNOB);

    lv_obj_clear_flag(ui->slider, LV_OBJ_FLAG_CLICKABLE);
}

lv_obj_t *CreateTextLabel(lv_obj_t *parent, const char *text, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, font, 0);
    return label;
}

void CreateSeekUi(lv_obj_t *root, play_ui_t *ui)
{
    // 上：ファイル名（左寄せ）
    ui->label_file = CreateTextLabel(root, "file_name.wav", &lv_font_montserrat_14);
    lv_obj_align_to(ui->label_file, ui->slider, LV_ALIGN_OUT_TOP_LEFT, 0, -10);

    // 下：経過（左）
    ui->label_pos = CreateTextLabel(root, "00:00", &lv_font_montserrat_14);
    lv_obj_align_to(ui->label_pos, ui->slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

    // 下：残り（右）
    ui->label_rem = CreateTextLabel(root, "-00:00", &lv_font_montserrat_14);
    lv_obj_align_to(ui->label_rem, ui->slider, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);
}

void FormatMmSs(char *dst, size_t dst_sz, uint32_t ms)
{
    const uint32_t sec = ms / 1000;
    const uint32_t m   = sec / 60;
    const uint32_t s   = sec % 60;
    std::snprintf(dst, dst_sz, "%02u:%02u", (unsigned)m, (unsigned)s);
}

void FormatRemain(char *dst, size_t dst_sz, uint32_t remain_ms)
{
    char t[8];
    FormatMmSs(t, sizeof(t), remain_ms);
    std::snprintf(dst, dst_sz, "-%s", t);
}

}  // namespace

void SetPlayViewState(play_ui_t *ui, play_view_state_t st)
{
    if (!ui || !ui->label_main) {
        return;
    }

    lv_label_set_text(ui->label_main, (st == play_view_state_t::ShowPause) ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

void SetPlaySeek(play_ui_t *ui, const char *filename, uint32_t position_ms, uint32_t duration_ms)
{
    if (!ui || !ui->slider) {
        return;
    }

    // filename
    if (ui->label_file && filename) {
        lv_label_set_text(ui->label_file, filename);
    }

    // slider
    if (duration_ms == 0) {
        lv_slider_set_range(ui->slider, 0, 1);
        lv_slider_set_value(ui->slider, 0, LV_ANIM_OFF);
    } else {
        if (position_ms > duration_ms)
            position_ms = duration_ms;
        lv_slider_set_range(ui->slider, 0, (int32_t)duration_ms);
        lv_slider_set_value(ui->slider, (int32_t)position_ms, LV_ANIM_OFF);
    }

    // time labels
    if (ui->label_pos) {
        char cur[8];
        FormatMmSs(cur, sizeof(cur), position_ms);
        lv_label_set_text(ui->label_pos, cur);
    }
    if (ui->label_rem) {
        uint32_t remain = (duration_ms > position_ms) ? (duration_ms - position_ms) : 0;
        char     rem[12];
        FormatRemain(rem, sizeof(rem), remain);
        lv_label_set_text(ui->label_rem, rem);
    }
}

/**
 * @brief Main（再生/停止）操作の通知コールバックを登録する
 *
 * @param ui      生成したUIハンドル群
 * @param on_main Main操作時に呼ばれる関数
 * @param user   OnMainに渡すuserポインタ
 */
void SetPlayMainCallback(play_ui_t *ui, nav_cb_t on_main, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_main = on_main;
    ui->cb_user = user;
}

/**
 * @brief Back操作の通知コールバックを登録する
 *
 * @param ui      生成したUIハンドル群
 * @param on_back Back 操作時に呼ばれる関数
 * @param user   OnBackに渡すuser
 */
void SetPlayBackCallback(play_ui_t *ui, nav_cb_t on_back, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_back = on_back;
    ui->cb_user = user;
}

/**
 * @brief Menu操作の通知コールバックを登録する
 *
 * @param ui      生成したUIハンドル群
 * @param on_menu Menu操作時に呼ばれる関数
 * @param user   OnMenuに渡すuserポインタ
 */
void SetPlayMenuCallback(play_ui_t *ui, nav_cb_t on_menu, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_menu = on_menu;
    ui->cb_user = user;
}

/**
 * @brief PLAY画面UIをparent上に生成する
 *
 * - 背景色を設定
 * - Back（右上 "PLAY"）、Menu（左上丸ボタン）、Main（中央丸ボタン）、Slider（下部）を生成
 * - 内部でuiをゼロクリアし、生成したlv_obj_t*をuiに格納する
 *
 * @param parent  生成先の親オブジェクト
 * @param[out] ui 生成したUIハンドル
 */
void CreatePlayUi(lv_obj_t *parent, play_ui_t *ui)
{
    if (!ui || !parent) {
        return;
    }

    memset(ui, 0, sizeof(*ui));

    lv_obj_t *root = parent;
    ui->root       = root;

    ApplyRootStylePlay(root);

    CreateBackButton(root, ui);

    CreateMenuButton(root, ui);

    CreateMainButton(root, ui);

    CreateBottomSlider(root, ui);

    CreateSeekUi(root, ui);

    CreatePlayListUi(root, &ui->file_sheet);
    SetPlayListSelectCallback(&ui->file_sheet, OnFileSelected, ui);

    // タイトルは一旦非表示
    lv_obj_add_flag(ui->label_title, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace gui
}  // namespace core1
