/**
 * @file record_screen.cpp
 * @brief 録音画面の生成とイベント処理
 *
 * LVGL上に録音画面を構築し、Back/Menuの操作をアプリ層へコールバック通知する。
 * Mainボタンは現状雛形として表示（▶/■）のみをトグルする。
 */

// 自ヘッダー
#include "record_screen.h"

// 標準ライブラリ
#include <cstdio>
#include <string.h>

// プロジェクトライブラリ
#include "screen_utility.h"

extern "C" {
extern const lv_font_t font_awesome;
}

namespace core1 {
namespace gui {
namespace {

constexpr const char *kIconMic = "\xEF\x84\xB0";  // U+F130 (Font Awesome: mic)
constexpr int32_t kMainButtonSize = 104;
constexpr int32_t kMainButtonOffsetY = -15;
constexpr int32_t kRippleMaxSize  = 220;
const lv_color_t  kRippleColor    = lv_color_hex(0xFF6B96);

/**
 * @brief Backボタンのイベント処理
 *
 * 画面遷移などの実処理はアプリ層で行う前提で、登録済みコールバックに通知する。
 *
 * @param event LVGLイベント
 */
void OnBack(lv_event_t *event)
{
    auto *ui = static_cast<record_ui_t *>(lv_event_get_user_data(event));
    if (!ui) {
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
    auto *ui = static_cast<record_ui_t *>(lv_event_get_user_data(event));
    if (!ui) {
        return;
    }

    if (ui->on_menu) {
        ui->on_menu(ui->cb_user);
    }
}

/**
 * @brief Mainボタンのイベント処理
 *
 * 現在はUI表示（▶/■）のみをトグルする。
 * 実際の録音開始/停止の制御は、後でアプリ層と接続する想定。
 *
 * @param event LVGLイベント
 */
void OnMain(lv_event_t *event)
{
    // ▶ / ■ 切替（録音開始/停止を後で繋ぐ）
    record_ui_t *ui = (record_ui_t *)lv_event_get_user_data(event);
    if (!ui || !ui->label_main) {
        return;
    }

    if (ui->on_main) {
        ui->on_main(ui->cb_user);
    }
}

/**
 * @brief 録音画面用のルート背景スタイルを適用する
 *
 * - 背景色（紫）
 * - rootのスクロール無効化
 *
 * @param root ルート（親）オブジェクト
 */
void ApplyRootStyleRecord(lv_obj_t *root)
{
    core1::gui::KillScroll(root);

    // 背景（Frame2：紫）
    const lv_color_t BG_REC = core1::gui::color::RecordBg();  // 見た目RGB: 紫
    lv_obj_set_style_bg_color(root, BG_REC, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
}

/**
 * @brief 透明なボタン（当たり判定用）の見た目を適用する
 *
 * @param btn 対象ボタン
 */
void ApplyTransparentButtonStyle(lv_obj_t *btn)
{
    core1::gui::KillScroll(btn);

    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
}

/**
 * @brief 丸ボタンの見た目を適用する
 *
 * @param btn 対象ボタン
 * @param bg  背景色
 * @param opa 背景不透明度
 */
void ApplyCircleButtonStyle(lv_obj_t *btn, lv_color_t bg, lv_opa_t opa)
{
    core1::gui::KillScroll(btn);

    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, opa, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
}

/**
 * @brief ボタン中央に白文字ラベルを生成する
 *
 * @param parent 親オブジェクト（ボタンなど）
 * @param text   表示文字列
 * @param font   使用フォント
 * @return 生成したラベル
 */
lv_obj_t *CreateCenteredLabel(lv_obj_t *parent, const char *text, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, core1::gui::color::White(), 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_center(label);

    return label;
}

void MainLabelOpacityExec(void *obj, int32_t value)
{
    if (!obj) {
        return;
    }
    lv_obj_set_style_text_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(value), 0);
}

void RippleSizeExec(void *obj, int32_t value)
{
    if (!obj) {
        return;
    }

    auto *ring = static_cast<lv_obj_t *>(obj);
    lv_obj_set_size(ring, value, value);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, kMainButtonOffsetY);
}

void RippleOpaExec(void *obj, int32_t value)
{
    if (!obj) {
        return;
    }
    lv_obj_set_style_bg_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(value), 0);
}

void MainButtonShadowWidthExec(void *obj, int32_t value)
{
    if (!obj) {
        return;
    }
    lv_obj_set_style_shadow_width(static_cast<lv_obj_t *>(obj), value, 0);
}

void MainButtonShadowOpaExec(void *obj, int32_t value)
{
    if (!obj) {
        return;
    }
    lv_obj_set_style_shadow_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(value), 0);
}

lv_obj_t *CreateRipple(lv_obj_t *root, lv_opa_t opa)
{
    lv_obj_t *ring = lv_obj_create(root);
    core1::gui::KillScroll(ring);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ring, kRippleColor, 0);
    lv_obj_set_style_bg_opa(ring, opa, 0);
    lv_obj_set_style_border_width(ring, 0, 0);
    lv_obj_set_style_shadow_width(ring, 0, 0);
    lv_obj_set_size(ring, kMainButtonSize, kMainButtonSize);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, kMainButtonOffsetY);
    lv_obj_add_flag(ring, LV_OBJ_FLAG_HIDDEN);
    return ring;
}

lv_obj_t *CreateTextLabel(lv_obj_t *parent, const char *text, const lv_font_t *font);

/**
 * @brief 左上RECボタンを生成する
 *
 * @param root ルート
 * @param ui   UIハンドル
 */
void CreateBackButton(lv_obj_t *root, record_ui_t *ui)
{
    lv_obj_t *btn_back = lv_button_create(root);
    ApplyTransparentButtonStyle(btn_back);

    lv_obj_set_size(btn_back, 110, 44);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_add_event_cb(btn_back, OnBack, LV_EVENT_CLICKED, ui);

    ui->label_title = CreateCenteredLabel(btn_back, "REC", &lv_font_montserrat_28);
}

/**
 * @brief 右上メニューボタン（"≡"）を生成する
 *
 * @param root ルート
 * @param ui   UIハンドル
 */
void CreateMenuButton(lv_obj_t *root, record_ui_t *ui)
{
    ui->btn_menu = lv_button_create(root);
    lv_obj_set_size(ui->btn_menu, 56, 56);
    lv_obj_align(ui->btn_menu, LV_ALIGN_TOP_RIGHT, -10, 10);
    ApplyCircleButtonStyle(ui->btn_menu, core1::gui::color::CircleButtonDark(), LV_OPA_40);
    lv_obj_add_event_cb(ui->btn_menu, OnMenu, LV_EVENT_PRESSED, ui);

#ifdef LV_SYMBOL_SETTINGS
    ui->label_menu = CreateCenteredLabel(ui->btn_menu, LV_SYMBOL_SETTINGS, &lv_font_montserrat_28);
#else
    ui->label_menu = CreateCenteredLabel(ui->btn_menu, LV_SYMBOL_LIST, &lv_font_montserrat_28);
#endif
    lv_obj_move_foreground(ui->btn_menu);
}

/**
 * @brief 中央のメインボタン（▶/■）を生成する
 *
 * @param root ルート
 * @param ui   UIハンドル
 */
void CreateMainButton(lv_obj_t *root, record_ui_t *ui)
{
    ui->ripple_b = CreateRipple(root, LV_OPA_90);
    ui->ripple_a = CreateRipple(root, static_cast<lv_opa_t>(120));

    ui->btn_main = lv_button_create(root);

    lv_obj_set_size(ui->btn_main, kMainButtonSize, kMainButtonSize);
    lv_obj_align(ui->btn_main, LV_ALIGN_CENTER, 0, kMainButtonOffsetY);

    ApplyCircleButtonStyle(ui->btn_main, core1::gui::color::CircleButtonRecord(), LV_OPA_COVER);
    lv_obj_set_style_shadow_color(ui->btn_main, kRippleColor, 0);
    lv_obj_set_style_shadow_width(ui->btn_main, 0, 0);
    lv_obj_set_style_shadow_opa(ui->btn_main, 0, 0);

    lv_obj_add_event_cb(ui->btn_main, OnMain, LV_EVENT_RELEASED, ui);

    // 待機状態は mic アイコンを初期表示
    ui->label_main = CreateCenteredLabel(ui->btn_main, kIconMic, &font_awesome);

    ui->label_status = CreateTextLabel(root, "", &lv_font_montserrat_16);
    lv_obj_set_style_text_opa(ui->label_status, LV_OPA_80, 0);
    lv_obj_align(ui->label_status, LV_ALIGN_CENTER, 0, 60);
}

/**
 * @brief 下部スライダー（白いバー）を生成する
 *
 * @param root ルート
 * @param ui   UIハンドル
 */
void CreateBottomSlider(lv_obj_t *root, record_ui_t *ui)
{
    ui->slider = lv_slider_create(root);
    core1::gui::KillScroll(ui->slider);

    lv_slider_set_range(ui->slider, 0, 1000);
    lv_obj_set_width(ui->slider, lv_pct(78));
    lv_obj_set_height(ui->slider, 8);
    lv_obj_align(ui->slider, LV_ALIGN_BOTTOM_MID, 0, -34);

    // 再生画面と同じ見た目
    lv_obj_set_style_bg_opa(ui->slider, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->slider, core1::gui::color::White(), LV_PART_MAIN);
    lv_obj_set_style_radius(ui->slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);

    lv_obj_set_style_bg_opa(ui->slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui->slider, core1::gui::color::White(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(ui->slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);

    lv_obj_set_style_opa(ui->slider, LV_OPA_0, LV_PART_KNOB);

    lv_obj_clear_flag(ui->slider, LV_OBJ_FLAG_CLICKABLE);
}

lv_obj_t *CreateTextLabel(lv_obj_t *parent, const char *text, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, core1::gui::color::White(), 0);
    lv_obj_set_style_text_font(label, font, 0);
    return label;
}

void CreateSeekUi(lv_obj_t *root, record_ui_t *ui)
{
    ui->label_pos = CreateTextLabel(root, "00:00", &lv_font_montserrat_14);
    lv_obj_align_to(ui->label_pos, ui->slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

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

void SetRecordViewState(record_ui_t *ui, record_view_state_t st)
{
    if (!ui || !ui->label_main) {
        return;
    }

    if (st == record_view_state_t::ShowStop) {
        lv_obj_set_style_text_font(ui->label_main, &lv_font_montserrat_28, 0);
        lv_label_set_text(ui->label_main, LV_SYMBOL_STOP);
        return;
    }

    lv_obj_set_style_text_font(ui->label_main, &font_awesome, 0);
    lv_label_set_text(ui->label_main, kIconMic);
}

void SetRecordStatusText(record_ui_t *ui, const char *text)
{
    if (!ui || !ui->label_status) {
        return;
    }

    lv_label_set_text(ui->label_status, (text && text[0] != '\0') ? text : "");
}

void SetRecordMainBlink(record_ui_t *ui, bool enable)
{
    if (!ui || !ui->label_main || !ui->ripple_a || !ui->ripple_b) {
        return;
    }

    lv_anim_del(ui->label_main, MainLabelOpacityExec);
    lv_obj_set_style_text_opa(ui->label_main, LV_OPA_COVER, 0);
    lv_anim_del(ui->btn_main, MainButtonShadowWidthExec);
    lv_anim_del(ui->btn_main, MainButtonShadowOpaExec);
    lv_anim_del(ui->ripple_a, RippleSizeExec);
    lv_anim_del(ui->ripple_a, RippleOpaExec);
    lv_anim_del(ui->ripple_b, RippleSizeExec);
    lv_anim_del(ui->ripple_b, RippleOpaExec);

    if (!enable) {
        lv_obj_add_flag(ui->ripple_a, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->ripple_b, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(ui->ripple_a, kMainButtonSize, kMainButtonSize);
        lv_obj_set_size(ui->ripple_b, kMainButtonSize, kMainButtonSize);
        lv_obj_set_style_bg_opa(ui->ripple_a, LV_OPA_0, 0);
        lv_obj_set_style_bg_opa(ui->ripple_b, LV_OPA_0, 0);
        lv_obj_set_style_shadow_width(ui->btn_main, 0, 0);
        lv_obj_set_style_shadow_opa(ui->btn_main, 0, 0);
        return;
    }

    lv_obj_clear_flag(ui->ripple_a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui->ripple_b, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_background(ui->ripple_b);
    lv_obj_move_foreground(ui->btn_main);

    lv_anim_t a_size;
    lv_anim_init(&a_size);
    lv_anim_set_var(&a_size, ui->ripple_a);
    lv_anim_set_exec_cb(&a_size, RippleSizeExec);
    lv_anim_set_values(&a_size, kMainButtonSize, kRippleMaxSize);
    lv_anim_set_time(&a_size, 900);
    lv_anim_set_repeat_count(&a_size, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a_size);

    lv_anim_t a_opa;
    lv_anim_init(&a_opa);
    lv_anim_set_var(&a_opa, ui->ripple_a);
    lv_anim_set_exec_cb(&a_opa, RippleOpaExec);
    lv_anim_set_values(&a_opa, 220, 0);
    lv_anim_set_time(&a_opa, 900);
    lv_anim_set_repeat_count(&a_opa, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a_opa);

    lv_anim_t b_size;
    lv_anim_init(&b_size);
    lv_anim_set_var(&b_size, ui->ripple_b);
    lv_anim_set_exec_cb(&b_size, RippleSizeExec);
    lv_anim_set_values(&b_size, kMainButtonSize, kRippleMaxSize);
    lv_anim_set_time(&b_size, 900);
    lv_anim_set_delay(&b_size, 450);
    lv_anim_set_repeat_count(&b_size, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&b_size);

    lv_anim_t b_opa;
    lv_anim_init(&b_opa);
    lv_anim_set_var(&b_opa, ui->ripple_b);
    lv_anim_set_exec_cb(&b_opa, RippleOpaExec);
    lv_anim_set_values(&b_opa, 170, 0);
    lv_anim_set_time(&b_opa, 900);
    lv_anim_set_delay(&b_opa, 450);
    lv_anim_set_repeat_count(&b_opa, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&b_opa);

    lv_anim_t shadow_w;
    lv_anim_init(&shadow_w);
    lv_anim_set_var(&shadow_w, ui->btn_main);
    lv_anim_set_exec_cb(&shadow_w, MainButtonShadowWidthExec);
    lv_anim_set_values(&shadow_w, 0, 64);
    lv_anim_set_time(&shadow_w, 500);
    lv_anim_set_playback_time(&shadow_w, 500);
    lv_anim_set_repeat_count(&shadow_w, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&shadow_w);

    lv_anim_t shadow_o;
    lv_anim_init(&shadow_o);
    lv_anim_set_var(&shadow_o, ui->btn_main);
    lv_anim_set_exec_cb(&shadow_o, MainButtonShadowOpaExec);
    lv_anim_set_values(&shadow_o, 220, 0);
    lv_anim_set_time(&shadow_o, 500);
    lv_anim_set_playback_time(&shadow_o, 500);
    lv_anim_set_repeat_count(&shadow_o, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&shadow_o);
}

void SetRecordSeek(record_ui_t *ui, uint32_t captured_ms, uint32_t target_ms)
{
    if (!ui || !ui->slider) {
        return;
    }

    if (target_ms == 0) {
        lv_slider_set_range(ui->slider, 0, 1);
        lv_slider_set_value(ui->slider, 0, LV_ANIM_OFF);
    } else {
        if (captured_ms > target_ms) {
            captured_ms = target_ms;
        }
        lv_slider_set_range(ui->slider, 0, (int32_t)target_ms);
        lv_slider_set_value(ui->slider, (int32_t)captured_ms, LV_ANIM_OFF);
    }

    if (ui->label_pos) {
        char cur[8];
        FormatMmSs(cur, sizeof(cur), captured_ms);
        lv_label_set_text(ui->label_pos, cur);
    }
    if (ui->label_rem) {
        const uint32_t remain = (target_ms > captured_ms) ? (target_ms - captured_ms) : 0;
        char           rem[12];
        FormatRemain(rem, sizeof(rem), remain);
        lv_label_set_text(ui->label_rem, rem);
    }
}

/**
 * @brief Back操作通知コールバックを登録する
 *
 * @param ui     生成したUIハンドル群
 * @param on_back Back操作時に呼ばれる関数
 * @param user    OnBackに渡すuser
 */
void SetRecordBackCallback(record_ui_t *ui, nav_cb_t on_back, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_back = on_back;
    ui->cb_user = user;
}

/**
 * @brief Menu操作通知コールバックを登録する
 *
 * @param ui     生成したUIハンドル群
 * @param on_menu Menu操作時に呼ばれる関数
 * @param user   OnMenuに渡すuser
 */
void SetRecordMenuCallback(record_ui_t *ui, nav_cb_t on_menu, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_menu = on_menu;
    ui->cb_user = user;
}

void SetRecordMainCallback(record_ui_t *ui, nav_cb_t on_main, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_main = on_main;
    ui->cb_user = user;
}

/**
 * @brief 録音画面UIをparent上に生成する
 *
 * - 背景（紫）
 * - 左上 "REC"（Back 扱いの透明ボタン）
 * - 右上メニュー（透明ボタン + "≡"）
 * - 中央大ボタン（▶/■ 表示）
 * - 下部スライダー（見た目寄せ：白いバー）
 *
 * @param parent  生成先の親オブジェクト
 * @param[out] ui 生成したUIハンドル群
 */
void CreateRecordUi(lv_obj_t *parent, record_ui_t *ui)
{
    if (!ui || !parent) {
        return;
    }

    memset(ui, 0, sizeof(*ui));

    lv_obj_t *root = parent;
    ui->root       = root;

    ApplyRootStyleRecord(root);

    CreateBackButton(root, ui);

    CreateMenuButton(root, ui);

    CreateMainButton(root, ui);

    CreateBottomSlider(root, ui);
    CreateSeekUi(root, ui);

    // タイトルは一旦非表示
    if (ui->label_title) {
        lv_obj_add_flag(ui->label_title, LV_OBJ_FLAG_HIDDEN);
    }
}
}  // namespace gui
}  // namespace core1
