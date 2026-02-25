/**
 * @file play_agc_screen.h
 * @brief 再生オプション画面のUI定義
 *
 * CreatePlayAgcUi()でUIを生成し、
 * GetPlayAgcParams()で現在設定をplay_options_params_tとして取得する。
 * 「完了」操作はSetPlayOptionsCallback()で登録したコールバックに通知される。
 */
#pragma once

// LVGLライブラリ
#include "lvgl.h"

namespace core1 {
namespace gui {

/**
 * @brief 再生オプション画面のパラメータ（UIから取得する設定値）
 */
struct play_agc_params_t
{
    bool    dist_link;      // 距離連動 ON/OFF
    int16_t dist_mm;        // 距離感度（スライダー値[mm]）
    int16_t min_gain_x100;  // 最小音量倍率(0.50x..2.00x を 50..200)
    int16_t max_gain_x100;  // 最大音量倍率(0.50x..2.00x を 50..200)
    int16_t speed_k;        // 追従速度パラメータ k（alpha = 1 / 2^k）
    int16_t speed_index;    // UI選択インデックス
};

typedef void (*play_agc_done_cb_t)(const play_agc_params_t *p, void *user);

/**
 * @brief 再生オプション画面のUIハンドル群
 *
 * CreatePlayAgcUi()がlv_obj_tを生成して本構造体へ格納する。
 * 画面の現在値はGetPlayOptionsParams()で取得する。
 */
struct play_agc_ui_t
{
    lv_obj_t *root;     // ルート（親）オブジェクト
    lv_obj_t *content;  // 縦スクロール領域

    lv_obj_t *sw_dist_link;  // 距離連動スイッチ

    lv_obj_t *slider_dist;        // 距離感度スライダー（mm）
    lv_obj_t *label_dist_value;   // 距離感度の現在値表示
    lv_obj_t *label_speed_value;  // 速度表示

    lv_obj_t *btn_min_minus;   // 最小音量 - ボタン
    lv_obj_t *btn_min_plus;    // 最小音量 + ボタン
    lv_obj_t *label_min_gain;  // 最小音量倍率表示

    lv_obj_t *btn_max_minus;   // 最大音量 - ボタン
    lv_obj_t *btn_max_plus;    // 最大音量 + ボタン
    lv_obj_t *label_max_gain;  // 最大音量倍率表示

    int16_t min_gain_x100;  // 最小音量倍率（x100表現）
    int16_t max_gain_x100;  // 最大音量倍率（x100表現）

    lv_obj_t *roller_speed;              // 速度選択ローラー
    lv_obj_t *label_roller_speed_value;  // 現在の k/α 表示

    lv_obj_t *btn_done;  // 完了ボタン

    play_agc_done_cb_t on_done{nullptr};
    void              *cb_user{nullptr};
};

void CreatePlayAgcUi(lv_obj_t *parent, play_agc_ui_t *ui);
void SetPlayAgcCallback(play_agc_ui_t *ui, play_agc_done_cb_t on_done, void *user);
void GetPlayAgcParams(const play_agc_ui_t *ui, play_agc_params_t *out);

}  // namespace gui
}  // namespace core1
