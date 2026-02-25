/**
 * @file home_screen.cpp
 * @brief ホーム画面のUI生成とイベント処理
 *
 * LVGL上にホーム画面を構築し、REC / PLAY押下イベントを
 * アプリ層へコールバック通知する。
 */

// 自ヘッダー
#include "home_screen.h"

// 標準ライブラリ
#include <cstdint>
#include <string.h>

namespace core1 {
namespace gui {
namespace {

/**
 * @brief RECボタン押下イベント
 *
 * @param event LVGLイベント
 */
void OnRec(lv_event_t *event)
{
    auto *ui = static_cast<home_ui_t *>(lv_event_get_user_data(event));
    if (!ui) {
        return;
    }

    if (ui->on_rec) {
        ui->on_rec(ui->cb_user);
    }
}

/**
 * @brief PLAYボタン押下イベント
 *
 * @param event LVGLイベント
 */
void OnPlay(lv_event_t *event)
{
    auto *ui = static_cast<home_ui_t *>(lv_event_get_user_data(event));
    if (!ui) {
        return;
    }

    if (ui->on_play) {
        ui->on_play(ui->cb_user);
    }
}

/**
 * @brief 画面外にはみ出す大きな円ボタンを生成する
 *
 * @param parent 親オブジェクト
 * @param x      左上X
 * @param y      左上Y
 * @param w      幅
 * @param h      高さ
 * @param bg     背景色
 * @return 生成したボタンオブジェクト
 */
lv_obj_t *CreateCircleBtn(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, lv_color_t bg)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);

    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);

    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    return btn;
}

/**
 * @brief 円ボタン直径を画面高さから算出する
 *
 * LVGL座標は整数なので、浮動小数点を使わずに計算する。
 */
lv_coord_t CalcCircleDiameter(lv_coord_t screen_h)
{
    // D = H * 1.55
    constexpr uint16_t kScaleNum = 155;
    constexpr uint16_t kScaleDen = 100;

    return (screen_h * kScaleNum) / kScaleDen;
}

/**
 * @brief ルート（親）に対して、ホーム画面共通のスタイルを適用する
 */
void ApplyRootStyle(lv_obj_t *root)
{
    // スクロール禁止
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_OFF);

    // 背景（中央の黒帯）
    lv_obj_set_style_bg_color(root, lv_color_hex(0x2B2B33), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
}

/**
 * @brief 透明な当たり判定用ボタンの見た目を適用する
 *
 * 文字はここに載せるが、背景は透明にして見た目は裏の円で表現する。
 */
void ApplyTransparentHitBoxStyle(lv_obj_t *btn)
{
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
}

/**
 * @brief ラベルを生成し、親の中央に配置する
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
 * @brief 背景の左右円（REC / PLAY）を生成する
 */
void CreateCirclePlates(lv_obj_t *root, lv_coord_t W, lv_coord_t H, home_ui_t *ui)
{
    // でかい正円を左右に配置して、画面で切り取る
    const lv_coord_t D = CalcCircleDiameter(H);
    const lv_coord_t Y = -(D - H) / 2;  // 垂直中央に見えるように

    const lv_coord_t gap     = 0;                      // 中央の黒い隙間の幅（とりあえず0にする）
    const lv_coord_t LEFT_X  = (W / 2 - gap / 2) - D;  // 左円の右端が中央 -gap / 2
    const lv_coord_t RIGHT_X = (W / 2 + gap / 2);      // 右円の左端が中央 +gap / 2

    const lv_color_t REC_BG  = lv_color_hex(0xE65A5A);
    const lv_color_t PLAY_BG = lv_color_hex(0xFF8600);

    ui->btn_rec  = CreateCircleBtn(root, LEFT_X, Y, D, D, REC_BG);
    ui->btn_play = CreateCircleBtn(root, RIGHT_X, Y, D, D, PLAY_BG);

    // クリック自体は拾えるようにしておく
    lv_obj_add_event_cb(ui->btn_rec, OnRec, LV_EVENT_CLICKED, ui);
    lv_obj_add_event_cb(ui->btn_play, OnPlay, LV_EVENT_CLICKED, ui);
}

/**
 * @brief 中央付近の透明ボタン＋ラベル（REC/PLAY）を生成する
 *
 * 円ボタンは画面外にはみ出すため、中心に置いたラベルが切れたり当たり判定が直感とズレたりする。
 * そのため、テキストは円の子ではなくroot上に配置し、当たり判定を安定させる。
 */
void CreateCenterTextButtons(lv_obj_t *root, home_ui_t *ui)
{
    // RECのテキストボタン
    ui->btn_rec_text = lv_button_create(root);
    // 当たり判定
    lv_obj_set_size(ui->btn_rec_text, 120, 60);
    // 位置はデザイン調整値
    lv_obj_align(ui->btn_rec_text, LV_ALIGN_CENTER, -40, 0);
    ApplyTransparentHitBoxStyle(ui->btn_rec_text);
    lv_obj_add_event_cb(ui->btn_rec_text, OnRec, LV_EVENT_CLICKED, ui);

    ui->label_rec = CreateCenteredLabel(ui->btn_rec_text, "REC", &lv_font_montserrat_28);

    // PLAYのテキストボタン
    ui->btn_play_text = lv_button_create(root);
    lv_obj_set_size(ui->btn_play_text, 140, 60);
    lv_obj_align(ui->btn_play_text, LV_ALIGN_CENTER, +40, 0);
    ApplyTransparentHitBoxStyle(ui->btn_play_text);
    lv_obj_add_event_cb(ui->btn_play_text, OnPlay, LV_EVENT_CLICKED, ui);

    ui->label_play = CreateCenteredLabel(ui->btn_play_text, "PLAY", &lv_font_montserrat_28);
}
}  // namespace

/**
 * @brief ホーム画面UIをparent上に生成する
 *
 * - 背景（中央の黒帯）を設定
 * - 左右に大きな円（REC / PLAY）を配置して画面で切り取る
 * - 文字はボタン内ではなくroot中央付近に透明ボタンとして配置し、当たり判定を確保する
 *
 * @param parent  生成先の親オブジェクト
 * @param[out] ui 生成したUIハンドル群
 */
void CreateHomeUi(lv_obj_t *parent, home_ui_t *ui)
{
    if (!ui || !parent) {
        return;
    }

    memset(ui, 0, sizeof(*ui));

    lv_obj_t *root = parent;
    ui->root       = root;

    const lv_coord_t W = lv_obj_get_width(root);
    const lv_coord_t H = lv_obj_get_height(root);

    ApplyRootStyle(root);

    CreateCirclePlates(root, W, H, ui);

    CreateCenterTextButtons(root, ui);
}

/**
 * @brief ホーム画面のナビゲーション（REC/PLAY）コールバックを登録する
 *
 * @param ui      生成したUIハンドル群
 * @param on_rec  REC押下時のコールバック
 * @param on_play PLAY押下時のコールバック
 * @param user   コールバックに渡すuserポインタ
 */
void SetHomeUiCallback(home_ui_t *ui, home_nav_cb_t on_rec, home_nav_cb_t on_play, void *user)
{
    if (!ui) {
        return;
    }

    ui->on_rec  = on_rec;
    ui->on_play = on_play;
    ui->cb_user = user;
}
}  // namespace gui
}  // namespace core1
