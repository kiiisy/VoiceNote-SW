/**
 * @file record_option_screen.cpp
 * @brief 録音オプション画面のUI実装
 *
 * DCカット/ノイズゲートの設定を、スイッチとドロップダウンで編集する画面を構築する。
 * 「完了」押下時に設定値を収集し、登録済みコールバックへ通知する。
 */

#include "record_option_screen.h"

#include <cstring>

#include "screen_utility.h"

namespace core1 {
namespace gui {
namespace {

static constexpr uint16_t kDcFcCandidatesHz[]       = {10, 20, 40, 80, 120};
static constexpr uint16_t kNgOpenCandidatesX1000[]  = {20, 40, 60, 80, 100, 150, 200};
static constexpr uint16_t kNgCloseCandidatesX1000[] = {10, 20, 30, 40, 60, 80, 120, 160};
static constexpr uint16_t kNgAttackCandidatesMs[]   = {1, 2, 5, 10, 20, 50, 100, 200};
static constexpr uint16_t kNgReleaseCandidatesMs[]  = {5, 10, 20, 50, 100, 200, 500, 1000};

/**
 * @brief 子オブジェクトから親へのスクロール伝播を有効化する
 *
 * @param obj 対象オブジェクト
 */
void EnableScrollBubble(lv_obj_t *obj)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
}

/**
 * @brief ルート背景スタイルを適用する
 *
 * @param root ルートオブジェクト
 */
void ApplyRootStyle(lv_obj_t *root)
{
    lv_obj_set_style_bg_color(root, LV_COLOR_RGB_AS_BGR(0x5A5AE6), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
}

/**
 * @brief 縦スクロール用のcontentコンテナを生成する
 *
 * @param root 親オブジェクト
 * @param width コンテナ幅
 * @return 生成したcontentオブジェクト
 */
lv_obj_t *CreateContent(lv_obj_t *root, lv_coord_t width)
{
    lv_obj_t *content = lv_obj_create(root);
    lv_obj_set_size(content, width, lv_pct(100));
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_top(content, 14, 0);
    lv_obj_set_style_pad_bottom(content, 12, 0);
    lv_obj_set_style_pad_left(content, 16, 0);
    lv_obj_set_style_pad_right(content, 16, 0);
    lv_obj_set_style_pad_row(content, 10, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    EnableScrollBubble(content);
    return content;
}

/**
 * @brief 1行分の横並びrowコンテナを生成する
 *
 * @param parent 親オブジェクト
 * @param h 行の高さ
 * @return 生成したrowオブジェクト
 */
lv_obj_t *CreateRow(lv_obj_t *parent, lv_coord_t h)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), h);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    EnableScrollBubble(row);
    return row;
}

/**
 * @brief テキストラベルを生成する
 *
 * @param parent 親オブジェクト
 * @param text 表示文字列
 * @param font 使用フォント
 * @return 生成したラベル
 */
lv_obj_t *CreateLabel(lv_obj_t *parent, const char *text, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, font, 0);
    return label;
}

/**
 * @brief 「完了」ボタンのイベント処理
 *
 * 現在のUI値を取得して登録済みコールバックへ通知する。
 *
 * @param event LVGLイベント
 */
void OnDone(lv_event_t *event)
{
    auto *ui = static_cast<rec_option_ui_t *>(lv_event_get_user_data(event));
    if (!ui || !ui->on_done) {
        return;
    }

    rec_option_params_t p{};
    GetRecOptionParams(ui, &p);
    ui->on_done(&p, ui->cb_user);
}

/**
 * @brief ドロップダウンを生成する
 *
 * @param parent 親オブジェクト
 * @param options 改行区切りの選択肢文字列
 * @param selected 初期選択インデックス
 * @return 生成したドロップダウン
 */
lv_obj_t *CreateDropDown(lv_obj_t *parent, const char *options, uint16_t selected)
{
    lv_obj_t *dd = lv_dropdown_create(parent);
    core1::gui::KillScroll(dd);
    lv_dropdown_set_options(dd, options);
    lv_dropdown_set_selected(dd, selected);
    lv_obj_set_style_text_font(dd, &noto_sans_jp, 0);
    lv_obj_set_style_text_color(dd, lv_color_black(), 0);
    lv_obj_set_style_bg_color(dd, lv_color_white(), 0);
    lv_obj_set_style_border_width(dd, 0, 0);
    lv_obj_set_style_radius(dd, 10, 0);
    lv_obj_set_style_pad_left(dd, 10, 0);
    lv_obj_set_style_pad_right(dd, 10, 0);

    lv_obj_t *list = lv_dropdown_get_list(dd);
    if (list) {
        lv_obj_set_style_bg_color(list, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_font(list, &noto_sans_jp, LV_PART_MAIN);
    }

    lv_obj_add_flag(dd, LV_OBJ_FLAG_EVENT_BUBBLE);
    return dd;
}

/**
 * @brief スイッチ行（タイトル + switch）を生成する
 *
 * @param content 親content
 * @param title 行タイトル
 * @param[out] out_switch 生成したswitch
 */
void CreateSwitchRow(lv_obj_t *content, const char *title, lv_obj_t **out_switch)
{
    lv_obj_t *row = CreateRow(content, 44);
    CreateLabel(row, title, &noto_sans_jp);
    *out_switch = lv_switch_create(row);
    core1::gui::KillScroll(*out_switch);
}

/**
 * @brief ドロップダウン行（タイトル + dropdown）を生成する
 *
 * @param content 親content
 * @param title 行タイトル
 * @param options 改行区切りの選択肢文字列
 * @param selected 初期選択インデックス
 * @param[out] out_dd 生成したdropdown
 * @param ui 未使用（シグネチャ互換）
 */
void CreateDropDownRow(lv_obj_t *content, const char *title, const char *options, uint16_t selected, lv_obj_t **out_dd,
                       rec_option_ui_t *)
{
    lv_obj_t *row = CreateRow(content, 44);
    lv_obj_t *label = CreateLabel(row, title, &noto_sans_jp);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_flex_grow(label, 1);
    lv_obj_set_width(label, 1);
    lv_obj_set_style_pad_right(label, 10, 0);

    *out_dd = CreateDropDown(row, options, selected);
    lv_obj_set_width(*out_dd, 120);
}

/**
 * @brief 完了ボタン行を生成する
 *
 * @param content 親content
 * @param ui UIハンドル
 */
void CreateRowDone(lv_obj_t *content, rec_option_ui_t *ui)
{
    lv_obj_t *row = CreateRow(content, 68);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ui->btn_done = lv_button_create(row);
    lv_obj_set_size(ui->btn_done, 110, 44);
    core1::gui::KillScroll(ui->btn_done);
    lv_obj_set_style_radius(ui->btn_done, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ui->btn_done, LV_COLOR_RGB_AS_BGR(0x6A5ACD), 0);
    lv_obj_add_event_cb(ui->btn_done, OnDone, LV_EVENT_RELEASED, ui);

    lv_obj_t *label = CreateLabel(ui->btn_done, "完了", &noto_sans_jp);
    lv_obj_center(label);
}

/**
 * @brief カットオフ周波数の選択indexから値(Hz)を取得する
 *
 * @param idx 選択index
 * @return 周波数[Hz]
 */
uint16_t GetDcFcHzByIndex(uint16_t idx)
{
    if (idx >= (sizeof(kDcFcCandidatesHz) / sizeof(kDcFcCandidatesHz[0]))) {
        return kDcFcCandidatesHz[1];
    }
    return kDcFcCandidatesHz[idx];
}

/**
 * @brief Open閾値の選択indexから値(x1000)を取得する
 *
 * @param idx 選択index
 * @return 閾値(x1000)
 */
uint16_t GetNgOpenByIndex(uint16_t idx)
{
    if (idx >= (sizeof(kNgOpenCandidatesX1000) / sizeof(kNgOpenCandidatesX1000[0]))) {
        return kNgOpenCandidatesX1000[1];
    }
    return kNgOpenCandidatesX1000[idx];
}

/**
 * @brief Close閾値の選択indexから値(x1000)を取得する
 *
 * @param idx 選択index
 * @return 閾値(x1000)
 */
uint16_t GetNgCloseByIndex(uint16_t idx)
{
    if (idx >= (sizeof(kNgCloseCandidatesX1000) / sizeof(kNgCloseCandidatesX1000[0]))) {
        return kNgCloseCandidatesX1000[3];
    }
    return kNgCloseCandidatesX1000[idx];
}

/**
 * @brief Attackの選択indexから値(ms)を取得する
 *
 * @param idx 選択index
 * @return Attack時間[ms]
 */
uint16_t GetNgAttackByIndex(uint16_t idx)
{
    if (idx >= (sizeof(kNgAttackCandidatesMs) / sizeof(kNgAttackCandidatesMs[0]))) {
        return kNgAttackCandidatesMs[2];
    }
    return kNgAttackCandidatesMs[idx];
}

/**
 * @brief Releaseの選択indexから値(ms)を取得する
 *
 * @param idx 選択index
 * @return Release時間[ms]
 */
uint16_t GetNgReleaseByIndex(uint16_t idx)
{
    if (idx >= (sizeof(kNgReleaseCandidatesMs) / sizeof(kNgReleaseCandidatesMs[0]))) {
        return kNgReleaseCandidatesMs[3];
    }
    return kNgReleaseCandidatesMs[idx];
}

}  // namespace

/**
 * @brief 録音オプション画面UIを生成する
 *
 * @param parent 生成先の親オブジェクト
 * @param[out] ui 生成したUIハンドル
 */
void CreateRecOptionUi(lv_obj_t *parent, rec_option_ui_t *ui)
{
    if (!parent || !ui) {
        return;
    }

    std::memset(ui, 0, sizeof(*ui));
    ui->root = parent;

    ApplyRootStyle(parent);

    ui->content = CreateContent(parent, lv_obj_get_width(parent));

    CreateSwitchRow(ui->content, "DCカット", &ui->sw_dc_enable);
    lv_obj_add_state(ui->sw_dc_enable, LV_STATE_CHECKED);

    CreateDropDownRow(ui->content, "カットオフ周波数", "10 Hz\n20 Hz\n40 Hz\n80 Hz\n120 Hz", 1, &ui->dd_dc_fc, ui);

    CreateSwitchRow(ui->content, "ノイズゲート", &ui->sw_ng_enable);

    CreateDropDownRow(ui->content, "Open閾値", "0.020\n0.040\n0.060\n0.080\n0.100\n0.150\n0.200", 1,
                      &ui->dd_ng_open, ui);
    CreateDropDownRow(ui->content, "Close閾値", "0.010\n0.020\n0.030\n0.040\n0.060\n0.080\n0.120\n0.160", 3,
                      &ui->dd_ng_close, ui);
    CreateDropDownRow(ui->content, "Attack", "1 ms\n2 ms\n5 ms\n10 ms\n20 ms\n50 ms\n100 ms\n200 ms", 2,
                      &ui->dd_ng_attack, ui);
    CreateDropDownRow(ui->content, "Release", "5 ms\n10 ms\n20 ms\n50 ms\n100 ms\n200 ms\n500 ms\n1000 ms", 3,
                      &ui->dd_ng_release, ui);

    CreateRowDone(ui->content, ui);
}

/**
 * @brief 「完了」操作の通知コールバックを登録する
 *
 * @param ui UIハンドル
 * @param on_done 完了時に呼ばれる関数
 * @param user ユーザデータ
 */
void SetRecOptionCallback(rec_option_ui_t *ui, rec_option_done_cb_t on_done, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_done = on_done;
    ui->cb_user = user;
}

/**
 * @brief Back操作通知コールバックを登録する
 *
 * 現在のrecord_option画面ではBackボタン非表示だが、API互換のため保持する。
 *
 * @param ui UIハンドル
 * @param on_back Back時に呼ばれる関数
 * @param user ユーザデータ
 */
void SetRecOptionBackCallback(rec_option_ui_t *ui, rec_option_back_cb_t on_back, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_back = on_back;
    ui->cb_user = user;
}

/**
 * @brief UIから現在の設定値を取得する
 *
 * @param ui UIハンドル
 * @param[out] out 取得先
 */
void GetRecOptionParams(const rec_option_ui_t *ui, rec_option_params_t *out)
{
    if (!ui || !out) {
        return;
    }

    out->dc_enable         = lv_obj_has_state(ui->sw_dc_enable, LV_STATE_CHECKED);
    out->dc_fc_hz          = GetDcFcHzByIndex(lv_dropdown_get_selected(ui->dd_dc_fc));
    out->ng_enable         = lv_obj_has_state(ui->sw_ng_enable, LV_STATE_CHECKED);
    out->ng_th_open_x1000  = GetNgOpenByIndex(lv_dropdown_get_selected(ui->dd_ng_open));
    out->ng_th_close_x1000 = GetNgCloseByIndex(lv_dropdown_get_selected(ui->dd_ng_close));
    out->ng_attack_ms      = GetNgAttackByIndex(lv_dropdown_get_selected(ui->dd_ng_attack));
    out->ng_release_ms     = GetNgReleaseByIndex(lv_dropdown_get_selected(ui->dd_ng_release));
}

}  // namespace gui
}  // namespace core1
