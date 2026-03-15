/**
 * @file play_options_screen.cpp
 * @brief 再生オプション画面のUI実装
 *
 * 距離連動のON/OFF、距離感度、最小/最大音量倍率、追従速度（k, alpha）を
 * LVGLの各ウィジェットで編集できる画面を構築する。
 *
 * 「完了」ボタン押下時に GetPlayAgcParams()で値を収集して
 * 登録済みのplay_options_done_cb_tへ通知する。
 */

// 自ヘッダー
#include "play_agc_screen.h"

// 標準ライブラリ
#include <string.h>

// プロジェクトライブラリ
#include "screen_utility.h"

namespace core1 {
namespace gui {
namespace {

/**
 * @brief 子オブジェクトから親（content）へスクロール操作を伝播させる設定
 *
 * 行コンテナやラベル等でドラッグしても、親の縦スクロールが開始されるようにする。
 *
 * @param obj 対象オブジェクト
 */
void EnableScrollBubble(lv_obj_t *obj)
{
    // 子でドラッグしても、親（content）のスクロールが始まるように
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);

    // スクロール伝播（縦）
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
}

/**
 * @brief ルート（親）へ再生オプション画面の共通スタイルを適用する
 *
 * - 背景色（青）を設定
 * - root自身はスクロールさせない（contentで縦スクロール）
 *
 * @param root ルート（親）オブジェクト
 */
void ApplyRootStyle(lv_obj_t *root)
{
    // 背景（青）
    lv_obj_set_style_bg_color(root, core1::gui::color::PlayBg(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    // root は固定
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);
}

/**
 * @brief 画面の縦スクロール領域（content）を生成する
 *
 * @param root 親（root）
 * @param W    contentの幅
 * @return 生成したcontentオブジェクト
 */
lv_obj_t *CreateContent(lv_obj_t *root, lv_coord_t W)
{
    lv_obj_t *content = lv_obj_create(root);

    lv_obj_set_size(content, W, lv_pct(100));
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);

    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);

    // 全体を縦に積む（行コンテナで管理）
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 10, 0);

    // どこでもスクロールしやすく
    EnableScrollBubble(content);

    return content;
}

/**
 * @brief 1行分のrowコンテナ（横並び flex）を生成する
 *
 * @param parent 親（通常は content）
 * @param w      行の幅
 * @param h      行の高さ
 * @return 生成したrowオブジェクト
 */
lv_obj_t *CreateRow(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, w, h);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_left(row, 24, 0);
    lv_obj_set_style_pad_right(row, 24, 0);
    lv_obj_set_style_pad_top(row, 10, 0);
    lv_obj_set_style_pad_bottom(row, 10, 0);

    // 行自体はスクロールしない（親のcontentがスクロール）
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // 横並び
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    EnableScrollBubble(row);

    return row;
}

/**
 * @brief 行の左側に配置するタイトルラベルを生成する
 *
 * @param parent 親（row）
 * @param txt    表示文字列
 * @param font   使用フォント
 * @return 生成したラベル
 */
lv_obj_t *CreateTitle(lv_obj_t *parent, const char *txt, const lv_font_t *font)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, txt);

    lv_obj_set_style_text_color(lbl, core1::gui::color::White(), 0);
    lv_obj_set_style_text_font(lbl, font, 0);

    lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);

    // ラベルが必要以上に幅を食わないようにする
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_flex_grow(lbl, 1);  // 余った幅をラベルが受ける
    lv_obj_set_width(lbl, 1);      // growが効くように

    EnableScrollBubble(lbl);

    return lbl;
}

/**
 * @brief 「完了」ボタン押下時のイベント処理
 *
 * UIの現在値をplay_options_get_params() で収集し、登録済みコールバックへ通知する。
 *
 * @param event LVGL イベント
 */
void OnBack(lv_event_t *event)
{
    auto *ui = (play_agc_ui_t *)lv_event_get_user_data(event);

    if (ui->on_done) {
        play_agc_params_t p{};
        GetPlayAgcParams(ui, &p);
        ui->on_done(&p, ui->cb_user);
    }
}

/**
 * @brief k 値に応じた速度の表示名（日本語）を返す
 *
 * @param k 速度パラメータ
 * @return 表示用文字列
 */
const char *GetSpeedName(uint8_t k)
{
    if (k <= 3) {
        return "すばやい";
    }
    if (k <= 5) {
        return "やや速い";
    }
    if (k <= 7) {
        return "標準";
    }
    if (k <= 9) {
        return "ややゆっくり";
    }

    return "ゆっくり";
}

/**
 * @brief ローラーの選択indexから速度パラメータkを得る
 *
 * 5段階の選択肢を想定し、範囲外の場合は標準にフォールバックする。
 *
 * @param idx rollerのindex
 * @return k値
 */
uint8_t GetSpeed(uint16_t idx)
{
    // 5段階
    static constexpr uint8_t kSpeed[] = {2, 3, 6, 8, 10};

    return kSpeed[(idx < 5) ? idx : 2];
}

/**
 * @brief 速度表示ラベル（k/α表記）を更新する
 *
 * rollerの選択状態からkを求め、alpha=1/2^k を表示する。
 *
 * @param ui UIハンドル
 */
void UpdateSpeedValueLabel(play_agc_ui_t *ui)
{
    if (!ui || !ui->roller_speed || !ui->label_speed_value) {
        return;
    }

    const uint16_t INDEX = lv_roller_get_selected(ui->roller_speed);
    const uint8_t  K     = GetSpeed(INDEX);

    // α = 1/2^k
    const uint16_t DENOM = static_cast<uint16_t>(1u << K);  // k<=10ならOK

    lv_label_set_text_fmt(ui->label_speed_value, "%s  (k=%d, a=1/%d)", GetSpeedName(K), K, DENOM);
}

/**
 * @brief 音量倍率（x100表現）をラベルへ "w.ffx" 形式で表示する
 *
 * @param lbl   対象ラベル
 * @param v_x100 0.50x..2.00x を 50..200 のように x100 で表現した値
 */
void SetGainX100Label(lv_obj_t *lbl, int16_t v_x100)
{
    if (!lbl) {
        return;
    }

    int32_t w = static_cast<int32_t>(v_x100) / 100;
    int32_t f = static_cast<int32_t>(v_x100) % 100;
    if (f < 0) {
        f = -f;
    }

    char s[16];
    lv_snprintf(s, sizeof(s), "%ld.%02ldx", static_cast<long>(w), static_cast<long>(f));
    lv_label_set_text(lbl, s);
}

/**
 * @brief int16の範囲クランプ
 *
 * @param vol  入力値
 * @param low  下限
 * @param high 上限
 * @return クランプ後の値
 */
int16_t Clamp16(int16_t vol, int16_t low, int16_t high)
{
    if (vol < low) {
        return low;
    }
    if (vol > high) {
        return high;
    }

    return vol;
}

/**
 * @brief 整数の比率でパーセンテージ相当を計算する
 *
 * v * (num/den) を整数演算で求めるユーティリティ。
 *
 * @param v   入力値
 * @param num 分子（例：80）
 * @param den 分母（例：100）
 * @return 計算結果
 */
lv_coord_t Percent(lv_coord_t v, uint16_t num, uint16_t den) { return static_cast<lv_coord_t>((v * num) / den); }

/**
 * @brief 「完了」ボタン押下時のイベント処理
 *
 * UIの現在値をGetPlayOptionsParams()で収集し、
 * 登録済みコールバックへ通知する。
 *
 * @param event LVGLイベント
 */
void OnDone(lv_event_t *event)
{
    auto *ui = (play_agc_ui_t *)lv_event_get_user_data(event);
    if (!ui) {
        return;
    }

    if (ui->on_done) {
        play_agc_params_t p{};
        GetPlayAgcParams(ui, &p);
        ui->on_done(&p, ui->cb_user);
    }
}

/**
 * @brief 距離感度スライダー値変更イベント
 *
 * slider_distの現在値（mm）を label_dist_value に反映する。
 *
 * @param event LVGLイベント
 */
void OnDistSliderChanged(lv_event_t *event)
{
    auto *ui = (play_agc_ui_t *)lv_event_get_user_data(event);
    if (!ui || !ui->slider_dist || !ui->label_dist_value) {
        return;
    }

    const int32_t v = lv_slider_get_value(ui->slider_dist);
    lv_label_set_text_fmt(ui->label_dist_value, "%ld mm", static_cast<long>(v));
}

/**
 * @brief 速度ローラー値変更イベント
 *
 * roller_speedの選択状態からkを求め、速度表示ラベルを更新する。
 *
 * @param event LVGLイベント
 */
void OnSpeedChanged(lv_event_t *event)
{
    auto *ui = (play_agc_ui_t *)lv_event_get_user_data(event);
    UpdateSpeedValueLabel(ui);
}

/**
 * @brief min_gain_x100とmax_gain_x100の大小関係を保証する
 *
 * min > max になった場合に値を揃えて破綻しないようにする。
 *
 * @param ui UIハンドル
 */
void EnforceGainOrder(play_agc_ui_t *ui)
{
    if (!ui) {
        return;
    }

    if (ui->min_gain_x100 > ui->max_gain_x100) {
        ui->max_gain_x100 = ui->min_gain_x100;
    }
    if (ui->max_gain_x100 < ui->min_gain_x100) {
        ui->min_gain_x100 = ui->max_gain_x100;
    }
}

/**
 * @brief 既存のラベル（min/max）が存在する場合のみ表示を更新する
 *
 * @param ui UIハンドル
 */
void UpdateGainLabels(play_agc_ui_t *ui)
{
    if (!ui) {
        return;
    }

    SetGainX100Label(ui->label_min_gain, ui->min_gain_x100);
    SetGainX100Label(ui->label_max_gain, ui->max_gain_x100);
}

/**
 * @brief 最小音量倍率をdelta_x100分だけ変更する（クランプ/整合/表示更新込み）
 *
 * @param ui         UI ハンドル
 * @param delta_x100 変更量（例：+5 なら +0.05x）
 */
void StepGainMin(play_agc_ui_t *ui, int16_t delta_x100)
{
    ui->min_gain_x100 = Clamp16((int16_t)(ui->min_gain_x100 + delta_x100), 50, 200);
    EnforceGainOrder(ui);
    UpdateGainLabels(ui);
}

/**
 * @brief 最大音量倍率をdelta_x100分だけ変更する（クランプ/整合/表示更新込み）
 *
 * @param ui         UIハンドル
 * @param delta_x100 変更量（例：-5 なら -0.05x）
 */
void StepGainMax(play_agc_ui_t *ui, int16_t delta_x100)
{
    ui->max_gain_x100 = Clamp16((int16_t)(ui->max_gain_x100 + delta_x100), 50, 200);
    EnforceGainOrder(ui);
    UpdateGainLabels(ui);
}

/**
 * @brief 最小音量倍率の[-]ボタンイベント
 *
 * @param event LVGLイベント
 */
void OnMinMinus(lv_event_t *event) { StepGainMin((play_agc_ui_t *)lv_event_get_user_data(event), -5); }

/**
 * @brief 最小音量倍率の[+]ボタンイベント
 *
 * @param event LVGLイベント
 */
void OnMinPlus(lv_event_t *event) { StepGainMin((play_agc_ui_t *)lv_event_get_user_data(event), +5); }

/**
 * @brief 最大音量倍率の[-]ボタンイベント
 *
 * @param event LVGLイベント
 */
void OnMaxMinus(lv_event_t *event) { StepGainMax((play_agc_ui_t *)lv_event_get_user_data(event), -5); }

/**
 * @brief 最大音量倍率の[+]ボタンイベント
 *
 * @param event LVGLイベント
 */
void OnMaxPlus(lv_event_t *event) { StepGainMax((play_agc_ui_t *)lv_event_get_user_data(event), +5); }

/**
 * @brief [-] 値 [+] のステッパーUIを生成する
 *
 * @param parent_row     親（row）
 * @param btn_w          ボタン幅
 * @param btn_h          ボタン高さ
 * @param[out] out_btn_minus  - ボタン
 * @param[out] out_label_value 値ラベル
 * @param[out] out_btn_plus   + ボタン
 * @return ステッパー全体のコンテナ
 */
lv_obj_t *CreateStepper(lv_obj_t *parent_row, lv_coord_t btn_w, lv_coord_t btn_h, lv_obj_t **out_btn_minus,
                        lv_obj_t **out_label_value, lv_obj_t **out_btn_plus)
{
    lv_obj_t *cont = lv_obj_create(parent_row);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 2, 0);  // ボタン - 値 - ボタン間の隙間
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    // [-]
    lv_obj_t *btn_m = lv_button_create(cont);
    lv_obj_set_size(btn_m, btn_w, btn_h);
    core1::gui::KillScroll(btn_m);
    lv_obj_set_style_radius(btn_m, 10, 0);
    lv_obj_set_style_bg_opa(btn_m, LV_OPA_70, 0);

    lv_obj_t *lbl_m = lv_label_create(btn_m);
    lv_label_set_text(lbl_m, "-");
    lv_obj_set_style_text_font(lbl_m, &noto_sans_jp, 0);
    lv_obj_center(lbl_m);

    // value
    lv_obj_t *lbl_v = lv_label_create(cont);
    lv_obj_set_style_text_color(lbl_v, core1::gui::color::White(), 0);
    lv_obj_set_style_text_font(lbl_v, &noto_sans_jp, 0);
    lv_label_set_text(lbl_v, "--");
    lv_obj_set_style_pad_left(lbl_v, 12, 0);
    lv_obj_set_style_pad_right(lbl_v, 12, 0);
    lv_label_set_long_mode(lbl_v, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl_v, 80);
    lv_obj_set_style_text_align(lbl_v, LV_TEXT_ALIGN_CENTER, 0);

    // [+]
    lv_obj_t *btn_p = lv_button_create(cont);
    lv_obj_set_size(btn_p, btn_w, btn_h);
    core1::gui::KillScroll(btn_p);
    lv_obj_set_style_radius(btn_p, 10, 0);
    lv_obj_set_style_bg_opa(btn_p, LV_OPA_70, 0);

    lv_obj_t *lbl_p = lv_label_create(btn_p);
    lv_label_set_text(lbl_p, "+");
    lv_obj_set_style_text_font(lbl_p, &noto_sans_jp, 0);
    lv_obj_center(lbl_p);

    *out_btn_minus   = btn_m;
    *out_label_value = lbl_v;
    *out_btn_plus    = btn_p;

    return cont;
}

/**
 * @brief 「距離連動」行（タイトル + switch）を生成する
 *
 * @param content 縦スクロール領域
 * @param W       行幅
 * @param ui      UIハンドル
 */
void CreateRowDistLink(lv_obj_t *content, lv_coord_t W, play_agc_ui_t *ui)
{
    lv_obj_t *row = CreateRow(content, W, 56);
    CreateTitle(row, "距離連動", &noto_sans_jp);

    ui->sw_dist_link = lv_switch_create(row);
    core1::gui::KillScroll(ui->sw_dist_link);
    lv_obj_set_size(ui->sw_dist_link, 80, 40);

    lv_obj_set_style_radius(ui->sw_dist_link, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(ui->sw_dist_link, 4, 0);
    lv_obj_set_style_radius(ui->sw_dist_link, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(ui->sw_dist_link, 4, LV_PART_INDICATOR);

    EnableScrollBubble(ui->sw_dist_link);
}

/**
 * @brief 「距離感度」行（タイトル + 現在値 + slider）を生成する
 *
 * 2行構成：
 * - 1行目：タイトル + 現在値表示
 * - 2行目：スライダー
 *
 * @param content 縦スクロール領域
 * @param W       行幅
 * @param ui      UIハンドル
 */
void CreateRowDistSlider(lv_obj_t *content, lv_coord_t W, play_agc_ui_t *ui)
{
    // タイトル行（右に現在値）
    lv_obj_t *row_title = CreateRow(content, W, 40);
    CreateTitle(row_title, "距離感度(mm)", &noto_sans_jp);

    ui->label_dist_value = lv_label_create(row_title);
    lv_obj_set_style_text_color(ui->label_dist_value, core1::gui::color::White(), 0);
    lv_obj_set_style_text_font(ui->label_dist_value, &noto_sans_jp, 0);
    lv_label_set_text(ui->label_dist_value, "-- mm");
    EnableScrollBubble(ui->label_dist_value);

    // スライダー行（縦並び）
    lv_obj_t *row_slider = CreateRow(content, W, 70);
    lv_obj_set_flex_flow(row_slider, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row_slider, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ui->slider_dist = lv_slider_create(row_slider);
    core1::gui::KillScroll(ui->slider_dist);

    lv_slider_set_range(ui->slider_dist, 200, 3000);
    lv_obj_set_width(ui->slider_dist, Percent(W, 80, 100));  // 80%

    // スライダー操作を優先するなら bubble を付けない方が自然なことが多い。
    // EnableScrollBubble(ui->slider_dist);

    lv_obj_add_event_cb(ui->slider_dist, OnDistSliderChanged, LV_EVENT_VALUE_CHANGED, ui);

    // 初期表示
    if (ui->slider_dist && ui->label_dist_value) {
        lv_label_set_text_fmt(ui->label_dist_value, "%d mm", lv_slider_get_value(ui->slider_dist));
    }
}

/**
 * @brief 「最小音量」行（タイトル + ステッパー）を生成する
 *
 * @param content 縦スクロール領域
 * @param W       行幅
 * @param ui      UIハンドル
 */
void CreateRowMinGain(lv_obj_t *content, lv_coord_t W, play_agc_ui_t *ui)
{
    lv_obj_t *row = CreateRow(content, W, 56);
    CreateTitle(row, "最小音量", &noto_sans_jp);

    constexpr lv_coord_t BW = 52;
    constexpr lv_coord_t BH = 40;

    CreateStepper(row, BW, BH, &ui->btn_min_minus, &ui->label_min_gain, &ui->btn_min_plus);

    lv_obj_add_event_cb(ui->btn_min_minus, OnMinMinus, LV_EVENT_CLICKED, ui);
    lv_obj_add_event_cb(ui->btn_min_plus, OnMinPlus, LV_EVENT_CLICKED, ui);

    UpdateGainLabels(ui);
}

/**
 * @brief 「最大音量」行（タイトル + ステッパー）を生成する
 *
 * @param content 縦スクロール領域
 * @param W       行幅
 * @param ui      UIハンドル
 */
void CreateRowMaxGain(lv_obj_t *content, lv_coord_t W, play_agc_ui_t *ui)
{
    lv_obj_t *row = CreateRow(content, W, 56);
    CreateTitle(row, "最大音量", &noto_sans_jp);

    constexpr lv_coord_t BW = 52;
    constexpr lv_coord_t BH = 40;

    CreateStepper(row, BW, BH, &ui->btn_max_minus, &ui->label_max_gain, &ui->btn_max_plus);

    lv_obj_add_event_cb(ui->btn_max_minus, OnMaxMinus, LV_EVENT_CLICKED, ui);
    lv_obj_add_event_cb(ui->btn_max_plus, OnMaxPlus, LV_EVENT_CLICKED, ui);

    UpdateGainLabels(ui);
}

/**
 * @brief 「音量変化スピード」行（タイトル + 現在値 + roller）を生成する
 *
 * 2行構成：
 * - 1行目：タイトル + 現在値表示（k/α）
 * - 2行目：ローラー
 *
 * @param content 縦スクロール領域
 * @param W       行幅
 * @param ui      UIハンドル
 */
void CreateRowSpeedRoller(lv_obj_t *content, lv_coord_t W, play_agc_ui_t *ui)
{
    // タイトル行（右に現在値）
    lv_obj_t *row_title = CreateRow(content, W, 40);
    CreateTitle(row_title, "音量変化スピード", &noto_sans_jp);

    ui->label_speed_value = lv_label_create(row_title);
    lv_obj_set_style_text_color(ui->label_speed_value, core1::gui::color::White(), 0);
    lv_obj_set_style_text_font(ui->label_speed_value, &noto_sans_jp, 0);
    lv_label_set_text(ui->label_speed_value, "--");
    EnableScrollBubble(ui->label_speed_value);

    // ローラー行（縦並び）
    lv_obj_t *row_roller = CreateRow(content, W, 110);
    lv_obj_set_flex_flow(row_roller, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row_roller, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ui->roller_speed = lv_roller_create(row_roller);
    core1::gui::KillScroll(ui->roller_speed);

    lv_obj_set_style_text_font(ui->roller_speed, &noto_sans_jp, 0);

    lv_roller_set_options(ui->roller_speed,
                          "すばやい\n"
                          "やや速い\n"
                          "標準\n"
                          "ややゆっくり\n"
                          "ゆっくり",
                          LV_ROLLER_MODE_NORMAL);

    lv_roller_set_visible_row_count(ui->roller_speed, 3);
    lv_obj_set_width(ui->roller_speed, Percent(W, 60, 100));

    lv_roller_set_selected(ui->roller_speed, 2, LV_ANIM_OFF);
    UpdateSpeedValueLabel(ui);

    lv_obj_add_event_cb(ui->roller_speed, OnSpeedChanged, LV_EVENT_VALUE_CHANGED, ui);
}

/**
 * @brief 「完了」行（完了ボタン + 下余白）を生成する
 *
 * @param content 縦スクロール領域
 * @param W       行幅
 * @param ui      UIハンドル
 */
void CreateRowDone(lv_obj_t *content, lv_coord_t W, play_agc_ui_t *ui)
{
    lv_obj_t *row = CreateRow(content, W, 80);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ui->btn_done = lv_button_create(row);
    lv_obj_set_size(ui->btn_done, 110, 44);
    core1::gui::KillScroll(ui->btn_done);

    lv_obj_set_style_radius(ui->btn_done, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ui->btn_done, core1::gui::color::DoneButtonBg(), 0);
    lv_obj_add_event_cb(ui->btn_done, OnDone, LV_EVENT_CLICKED, ui);

    lv_obj_t *lbl_done = lv_label_create(ui->btn_done);
    lv_label_set_text(lbl_done, "完了");
    lv_obj_set_style_text_color(lbl_done, core1::gui::color::White(), 0);
    lv_obj_set_style_text_font(lbl_done, &noto_sans_jp, 0);
    lv_obj_center(lbl_done);

    // 下余白
    lv_obj_t *sp = lv_obj_create(content);
    lv_obj_set_size(sp, 1, 24);
    lv_obj_set_style_bg_opa(sp, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sp, 0, 0);
    lv_obj_clear_flag(sp, LV_OBJ_FLAG_SCROLLABLE);
    EnableScrollBubble(sp);
}
}  // namespace

/**
 * @brief UIから現在の設定値を取得する
 *
 * @param ui   UIハンドル
 * @param[out] out 取得先
 */
void GetPlayAgcParams(const play_agc_ui_t *ui, play_agc_params_t *out)
{
    if (!ui || !out) {
        return;
    }

    out->min_gain_x100 = ui->min_gain_x100;
    out->max_gain_x100 = ui->max_gain_x100;

    out->dist_link = (ui->sw_dist_link) ? lv_obj_has_state(ui->sw_dist_link, LV_STATE_CHECKED) : false;

    out->dist_mm = (ui->slider_dist) ? (int16_t)lv_slider_get_value(ui->slider_dist) : 0;

    out->speed_index = (ui->roller_speed) ? (int16_t)lv_roller_get_selected(ui->roller_speed) : 2;

    out->speed_k = GetSpeed((uint16_t)out->speed_index);
}

/**
 * @brief 「完了（戻る）」操作の通知コールバックを登録する
 *
 * @param on_done 完了時に呼ばれる関数
 * @param user    on_doneに渡すuser
 */
void SetPlayAgcCallback(play_agc_ui_t *ui, play_agc_done_cb_t on_done, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_done = on_done;
    ui->cb_user = user;
}

/**
 * @brief 再生オプション画面UIをparent上に生成する
 *
 * - 背景設定（青）
 * - 縦スクロール領域contentを生成
 * - 距離連動スイッチ、距離感度スライダー、最小/最大音量ステッパー、速度ローラー、完了ボタンを生成
 *
 * @param parent 生成先
 * @param[out] ui 生成したUIハンドル群
 */
void CreatePlayAgcUi(lv_obj_t *parent, play_agc_ui_t *ui)
{
    if (!ui || !parent) {
        return;
    }

    memset(ui, 0, sizeof(*ui));
    ui->root = parent;

    // 初期値（UI表示もこれに合わせる）
    ui->min_gain_x100 = 50;
    ui->max_gain_x100 = 150;

    ApplyRootStyle(ui->root);

    const lv_coord_t W = lv_obj_get_width(ui->root);

    ui->content = CreateContent(ui->root, W);

    CreateRowDistLink(ui->content, W, ui);

    CreateRowDistSlider(ui->content, W, ui);

    CreateRowMinGain(ui->content, W, ui);

    CreateRowMaxGain(ui->content, W, ui);

    CreateRowSpeedRoller(ui->content, W, ui);

    CreateRowDone(ui->content, W, ui);

    // 初期表示の整合
    UpdateGainLabels(ui);
    UpdateSpeedValueLabel(ui);

    if (ui->slider_dist && ui->label_dist_value) {
        lv_label_set_text_fmt(ui->label_dist_value, "%d mm", lv_slider_get_value(ui->slider_dist));
    }
}

}  // namespace gui
}  // namespace core1
