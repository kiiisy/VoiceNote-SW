/**
 * @file ui_transitions.cpp
 * @brief UiTransitionsの実装
 */

// 自ヘッダー
#include "ui_transitions.h"

// 標準ライブラリ
#include <cstdio>

// プロジェクトライブラリ
#include "logger_core.h"
#include "screen_utility.h"
#include "ui_navigator.h"

namespace core1 {
namespace gui {

void UiTransitions::OnBackToHomeFromPlayAnimDone(lv_anim_t *animation)
{
    auto *ctx = (BackRecCtx *)animation->user_data;
    if (!ctx || !ctx->self) {
        return;
    }

    auto *self = ctx->self;

    if (ctx->ov_label) {
        lv_obj_del(ctx->ov_label);
    }

    // PLAYを復帰
    lv_obj_clear_flag(self->store_.GetHomeUi().label_play, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(self->store_.GetPlayUi().label_title, LV_OBJ_FLAG_HIDDEN);

    self->nav_.SetCurrent(UiNavigator::Screen::Home);

    delete ctx;
}

void UiTransitions::OnBackToHomeFromRecAnimDone(lv_anim_t *animation)
{
    auto *ctx = (BackRecCtx *)animation->user_data;
    if (!ctx || !ctx->self) {
        return;
    }

    auto *self = ctx->self;

    if (ctx->ov_label) {
        lv_obj_del(ctx->ov_label);
    }

    // RECを復帰
    lv_obj_clear_flag(self->store_.GetHomeUi().label_rec, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(self->store_.GetRecUi().label_title, LV_OBJ_FLAG_HIDDEN);

    self->nav_.SetCurrent(UiNavigator::Screen::Home);

    delete ctx;
}

void UiTransitions::OnHomeToPlayAnimDone(lv_anim_t *animation)
{
    auto *ctx = (HomeToPlayAnimCtx *)animation->user_data;
    if (!ctx || !ctx->self) {
        return;
    }

    auto *self = ctx->self;

    // Play画面側の本物PLAYを表示
    lv_obj_clear_flag(self->store_.GetPlayUi().label_title, LV_OBJ_FLAG_HIDDEN);

    if (ctx->ov_circle)
        lv_obj_del(ctx->ov_circle);
    if (ctx->ov_label)
        lv_obj_del(ctx->ov_label);

    self->nav_.SetCurrent(UiNavigator::Screen::Play);

    delete ctx;
}

void UiTransitions::OnHomeToRecAnimDone(lv_anim_t *animation)
{
    auto *ctx = (HomeToRecAnimCtx *)animation->user_data;
    if (!ctx || !ctx->self) {
        return;
    }

    auto *self = ctx->self;

    // Rec画面側の本物RECを出す
    lv_obj_clear_flag(self->store_.GetRecUi().label_title, LV_OBJ_FLAG_HIDDEN);

    if (ctx->ov_circle)
        lv_obj_del(ctx->ov_circle);
    if (ctx->ov_label)
        lv_obj_del(ctx->ov_label);

    self->nav_.SetCurrent(UiNavigator::Screen::Recorder);

    delete ctx;
}

void UiTransitions::HomeToRec()
{
    // 録音画面の本物RECは一旦隠す
    lv_obj_add_flag(store_.GetRecUi().label_title, LV_OBJ_FLAG_HIDDEN);

    // ホームの紫円の位置・サイズ
    lv_area_t a_c;
    lv_obj_get_coords(store_.GetHomeUi().btn_rec, &a_c);
    lv_coord_t cx = a_c.x1;
    lv_coord_t cy = a_c.y1;
    lv_coord_t cw = (a_c.x2 - a_c.x1 + 1);
    lv_coord_t ch = (a_c.y2 - a_c.y1 + 1);

    // ホームのREC文字の位置
    lv_area_t a_t;
    lv_obj_get_coords(store_.GetHomeUi().label_rec, &a_t);
    lv_coord_t tx = a_t.x1;
    lv_coord_t ty = a_t.y1;

    // overlay layer
    lv_obj_t *layer = lv_layer_top();

    // ダミー円
    lv_obj_t *ov_circle = lv_obj_create(layer);
    lv_obj_set_pos(ov_circle, cx, cy);
    lv_obj_set_size(ov_circle, cw, ch);
    lv_obj_set_style_radius(ov_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ov_circle, 0, 0);
    lv_obj_set_style_shadow_width(ov_circle, 0, 0);
    lv_obj_set_style_bg_color(ov_circle, LV_COLOR_RGB_AS_BGR(0x5A5AE6), 0);
    lv_obj_set_style_bg_opa(ov_circle, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ov_circle, LV_OBJ_FLAG_SCROLLABLE);

    // ダミーREC文字
    lv_obj_t *ov_label = lv_label_create(layer);
    lv_label_set_text(ov_label, "REC");
    lv_obj_set_style_text_color(ov_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(ov_label, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ov_label, tx, ty);

    // ホームの本物を隠す
    lv_obj_add_flag(store_.GetHomeUi().btn_rec, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(store_.GetHomeUi().label_rec, LV_OBJ_FLAG_HIDDEN);

    // 紫円を右へスライド
    lv_coord_t     W            = lv_obj_get_width(store_.GetHomeScreen());
    lv_coord_t     circle_end_x = W;
    const uint32_t dur1         = 160;

    lv_anim_t a1;
    lv_anim_init(&a1);
    lv_anim_set_var(&a1, ov_circle);
    lv_anim_set_exec_cb(&a1, core1::gui::SetAnimX);
    lv_anim_set_values(&a1, cx, circle_end_x);
    lv_anim_set_time(&a1, dur1);
    lv_anim_set_path_cb(&a1, lv_anim_path_ease_in);
    lv_anim_start(&a1);

    // 録音画面を左からIN
    const uint32_t dur2 = 180;
    lv_screen_load_anim(store_.GetRecScreen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, dur2, 0, false);

    // REC文字を左上へ移動
    lv_coord_t dst_x = 8 + 10;
    lv_coord_t dst_y = 8 + 8;

    auto *ctx = new HomeToRecAnimCtx{this, ov_circle, ov_label};

    lv_anim_t ax, ay;
    lv_anim_init(&ax);
    lv_anim_set_var(&ax, ov_label);
    lv_anim_set_exec_cb(&ax, core1::gui::SetAnimX);
    lv_anim_set_values(&ax, tx, dst_x);
    lv_anim_set_time(&ax, dur2);
    lv_anim_set_path_cb(&ax, lv_anim_path_ease_out);
    lv_anim_start(&ax);

    lv_anim_init(&ay);
    lv_anim_set_var(&ay, ov_label);
    lv_anim_set_exec_cb(&ay, core1::gui::SetAnimY);
    lv_anim_set_values(&ay, ty, dst_y);
    lv_anim_set_time(&ay, dur2);
    lv_anim_set_path_cb(&ay, lv_anim_path_ease_out);
    lv_anim_set_user_data(&ay, ctx);
    lv_anim_set_ready_cb(&ay, OnHomeToRecAnimDone);
    lv_anim_start(&ay);

    // ホーム本物を戻す
    lv_obj_clear_flag(store_.GetHomeUi().btn_rec, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(store_.GetHomeUi().label_rec, LV_OBJ_FLAG_HIDDEN);
}

void UiTransitions::HomeToPlay()
{
    // 再生画面側の本物PLAYは一旦隠す
    lv_obj_add_flag(store_.GetPlayUi().label_title, LV_OBJ_FLAG_HIDDEN);

    // ホームの青円座標
    lv_area_t a_c;
    lv_obj_get_coords(store_.GetHomeUi().btn_play, &a_c);
    lv_coord_t cx = a_c.x1;
    lv_coord_t cy = a_c.y1;
    lv_coord_t cw = (a_c.x2 - a_c.x1 + 1);
    lv_coord_t ch = (a_c.y2 - a_c.y1 + 1);

    // ホームのPLAY文字座標
    lv_area_t a_t;
    lv_obj_get_coords(store_.GetHomeUi().label_play, &a_t);
    lv_coord_t tx = a_t.x1;
    lv_coord_t ty = a_t.y1;

    lv_obj_t *layer = lv_layer_top();

    // 青円（ダミー）
    lv_obj_t *ov_circle = lv_obj_create(layer);
    lv_obj_set_pos(ov_circle, cx, cy);
    lv_obj_set_size(ov_circle, cw, ch);
    lv_obj_set_style_radius(ov_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ov_circle, 0, 0);
    lv_obj_set_style_shadow_width(ov_circle, 0, 0);
    lv_obj_set_style_bg_color(ov_circle, LV_COLOR_RGB_AS_BGR(0x0086FF), 0);
    lv_obj_set_style_bg_opa(ov_circle, LV_OPA_COVER, 0);

    // PLAY文字（ダミー）
    lv_obj_t *ov_label = lv_label_create(layer);
    lv_label_set_text(ov_label, "PLAY");
    lv_obj_set_style_text_color(ov_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(ov_label, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(ov_label, tx, ty);

    // ホーム側本物を隠す
    lv_obj_add_flag(store_.GetHomeUi().btn_play, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(store_.GetHomeUi().label_play, LV_OBJ_FLAG_HIDDEN);

    // 青円を左へ抜け
    lv_coord_t     end_x = -cw;
    const uint32_t dur1  = 160;

    lv_anim_t a1;
    lv_anim_init(&a1);
    lv_anim_set_var(&a1, ov_circle);
    lv_anim_set_exec_cb(&a1, core1::gui::SetAnimX);
    lv_anim_set_values(&a1, cx, end_x);
    lv_anim_set_time(&a1, dur1);
    lv_anim_set_path_cb(&a1, lv_anim_path_ease_in);
    lv_anim_start(&a1);

    // 再生画面を右→左にIN
    const uint32_t dur2 = 180;
    lv_screen_load_anim(store_.GetPlayScreen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, dur2, 0, false);

    // PLAY文字を右上へ移動
    lv_coord_t dst_x = lv_obj_get_width(store_.GetHomeScreen()) - 8 - 110;
    lv_coord_t dst_y = 8 + 8;

    auto *ctx = new HomeToPlayAnimCtx{this, ov_circle, ov_label};

    lv_anim_t ax, ay;
    lv_anim_init(&ax);
    lv_anim_set_var(&ax, ov_label);
    lv_anim_set_exec_cb(&ax, core1::gui::SetAnimX);
    lv_anim_set_values(&ax, tx, dst_x);
    lv_anim_set_time(&ax, dur2);
    lv_anim_set_path_cb(&ax, lv_anim_path_ease_out);
    lv_anim_start(&ax);

    lv_anim_init(&ay);
    lv_anim_set_var(&ay, ov_label);
    lv_anim_set_exec_cb(&ay, core1::gui::SetAnimY);
    lv_anim_set_values(&ay, ty, dst_y);
    lv_anim_set_time(&ay, dur2);
    lv_anim_set_path_cb(&ay, lv_anim_path_ease_out);
    lv_anim_set_user_data(&ay, ctx);
    lv_anim_set_ready_cb(&ay, OnHomeToPlayAnimDone);
    lv_anim_start(&ay);

    // ホーム復帰用に戻す
    lv_obj_clear_flag(store_.GetHomeUi().btn_play, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(store_.GetHomeUi().label_play, LV_OBJ_FLAG_HIDDEN);
}

void UiTransitions::RecToHome()
{
    // 開始点（録音画面の左上REC文字）: グローバル座標
    lv_area_t src_t;
    lv_obj_get_coords(store_.GetRecUi().label_title, &src_t);

    const uint32_t dur = 180;

    // ホームを右→左で入れる（戻り）
    lv_screen_load_anim(store_.GetHomeScreen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, dur, 0, false);

    // overlay
    lv_obj_t *layer = lv_layer_top();
    lv_obj_t *ov    = lv_label_create(layer);
    lv_label_set_text(ov, "REC");
    lv_obj_set_style_text_color(ov, lv_color_white(), 0);
    lv_obj_set_style_text_font(ov, &lv_font_montserrat_28, 0);

    // overlay開始位置
    lv_obj_set_pos(ov, src_t.x1, src_t.y1);
    lv_obj_update_layout(ov);

    // dst は home_ui_.btn_rec_text のローカル座標から
    lv_obj_update_layout(store_.GetHomeScreen());
    lv_obj_update_layout(store_.GetHomeUi().btn_rec_text);

    const lv_coord_t btn_x = lv_obj_get_x(store_.GetHomeUi().btn_rec_text);
    const lv_coord_t btn_y = lv_obj_get_y(store_.GetHomeUi().btn_rec_text);
    const lv_coord_t btn_w = lv_obj_get_width(store_.GetHomeUi().btn_rec_text);
    const lv_coord_t btn_h = lv_obj_get_height(store_.GetHomeUi().btn_rec_text);

    const lv_coord_t text_w = lv_obj_get_width(ov);
    const lv_coord_t text_h = lv_obj_get_height(ov);

    const lv_coord_t dst_x = btn_x + (btn_w - text_w) / 2;
    const lv_coord_t dst_y = btn_y + (btn_h - text_h) / 2;

    // 本物を隠す
    lv_obj_add_flag(store_.GetHomeUi().label_rec, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(store_.GetRecUi().label_title, LV_OBJ_FLAG_HIDDEN);

    lv_anim_t ax, ay;

    lv_anim_init(&ax);
    lv_anim_set_var(&ax, ov);
    lv_anim_set_exec_cb(&ax, core1::gui::SetAnimX);
    lv_anim_set_values(&ax, src_t.x1, dst_x);
    lv_anim_set_time(&ax, dur);
    lv_anim_set_path_cb(&ax, lv_anim_path_ease_out);
    lv_anim_start(&ax);

    auto *ctx = new BackRecCtx{this, nullptr, ov};

    lv_anim_init(&ay);
    lv_anim_set_var(&ay, ov);
    lv_anim_set_exec_cb(&ay, core1::gui::SetAnimY);
    lv_anim_set_values(&ay, src_t.y1, dst_y);
    lv_anim_set_time(&ay, dur);
    lv_anim_set_path_cb(&ay, lv_anim_path_ease_out);
    lv_anim_set_user_data(&ay, ctx);
    lv_anim_set_ready_cb(&ay, OnBackToHomeFromRecAnimDone);
    lv_anim_start(&ay);
}

void UiTransitions::PlayToHome()
{
    // 開始点（再生画面の右上 PLAY 文字）: グローバル座標
    lv_area_t src_t;
    lv_obj_get_coords(store_.GetPlayUi().label_title, &src_t);

    const uint32_t dur = 180;

    // ホームを左→右で入れる
    lv_screen_load_anim(store_.GetHomeScreen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, dur, 0, false);

    // overlay
    lv_obj_t *layer = lv_layer_top();
    lv_obj_t *ov    = lv_label_create(layer);
    lv_label_set_text(ov, "PLAY");
    lv_obj_set_style_text_color(ov, lv_color_white(), 0);
    lv_obj_set_style_text_font(ov, &lv_font_montserrat_28, 0);

    // overlay開始位置
    lv_obj_set_pos(ov, src_t.x1, src_t.y1);
    lv_obj_update_layout(ov);

    // dst は home_ui_.btn_play_text のローカル座標から
    lv_obj_update_layout(store_.GetHomeScreen());
    lv_obj_update_layout(store_.GetHomeUi().btn_play_text);

    const lv_coord_t btn_x = lv_obj_get_x(store_.GetHomeUi().btn_play_text);
    const lv_coord_t btn_y = lv_obj_get_y(store_.GetHomeUi().btn_play_text);
    const lv_coord_t btn_w = lv_obj_get_width(store_.GetHomeUi().btn_play_text);
    const lv_coord_t btn_h = lv_obj_get_height(store_.GetHomeUi().btn_play_text);

    const lv_coord_t text_w = lv_obj_get_width(ov);
    const lv_coord_t text_h = lv_obj_get_height(ov);

    const lv_coord_t dst_x = btn_x + (btn_w - text_w) / 2;
    const lv_coord_t dst_y = btn_y + (btn_h - text_h) / 2;

    // 本物を隠す
    lv_obj_add_flag(store_.GetHomeUi().label_play, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(store_.GetPlayUi().label_title, LV_OBJ_FLAG_HIDDEN);

    // overlayを移動
    lv_anim_t ax, ay;

    lv_anim_init(&ax);
    lv_anim_set_var(&ax, ov);
    lv_anim_set_exec_cb(&ax, core1::gui::SetAnimX);
    lv_anim_set_values(&ax, src_t.x1, dst_x);
    lv_anim_set_time(&ax, dur);
    lv_anim_set_path_cb(&ax, lv_anim_path_ease_out);
    lv_anim_start(&ax);

    auto *ctx = new BackRecCtx{this, nullptr, ov};

    lv_anim_init(&ay);
    lv_anim_set_var(&ay, ov);
    lv_anim_set_exec_cb(&ay, core1::gui::SetAnimY);
    lv_anim_set_values(&ay, src_t.y1, dst_y);
    lv_anim_set_time(&ay, dur);
    lv_anim_set_path_cb(&ay, lv_anim_path_ease_out);
    lv_anim_set_user_data(&ay, ctx);
    lv_anim_set_ready_cb(&ay, OnBackToHomeFromPlayAnimDone);
    lv_anim_start(&ay);
}

}  // namespace gui
}  // namespace core1
