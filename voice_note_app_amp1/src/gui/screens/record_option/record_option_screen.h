/**
 * @file record_option_screen.h
 * @brief 録音のオプション画面のUI定義
 */
#pragma once

// LVGLライブラリ
#include "lvgl.h"

namespace core1 {
namespace gui {

struct rec_option_params_t
{
    bool     dc_enable{true};
    uint16_t dc_fc_hz{20};
    bool     ng_enable{false};
    uint16_t ng_th_open_x1000{50};
    uint16_t ng_th_close_x1000{40};
    uint16_t ng_attack_ms{5};
    uint16_t ng_release_ms{50};
    bool     arec_enable{true};
    uint16_t arec_threshold{0x0200};
    uint8_t  arec_window_shift{6};
    uint16_t arec_pretrig_samples{512};
    uint8_t  arec_required_windows{2};
};

typedef void (*rec_option_done_cb_t)(const rec_option_params_t *p, void *user);
typedef void (*rec_option_back_cb_t)(void *user);

struct rec_option_ui_t
{
    lv_obj_t *root{nullptr};
    lv_obj_t *content{nullptr};

    lv_obj_t *btn_back{nullptr};
    lv_obj_t *btn_done{nullptr};

    lv_obj_t *sw_dc_enable{nullptr};
    lv_obj_t *dd_dc_fc{nullptr};

    lv_obj_t *sw_ng_enable{nullptr};
    lv_obj_t *dd_ng_open{nullptr};
    lv_obj_t *dd_ng_close{nullptr};
    lv_obj_t *dd_ng_attack{nullptr};
    lv_obj_t *dd_ng_release{nullptr};

    lv_obj_t *sw_arec_enable{nullptr};
    lv_obj_t *dd_arec_threshold{nullptr};
    lv_obj_t *dd_arec_window_shift{nullptr};
    lv_obj_t *dd_arec_pretrig_samples{nullptr};
    lv_obj_t *dd_arec_required_windows{nullptr};

    rec_option_done_cb_t on_done{nullptr};
    rec_option_back_cb_t on_back{nullptr};
    void                *cb_user{nullptr};
};

void CreateRecOptionUi(lv_obj_t *parent, rec_option_ui_t *ui);
void SetRecOptionCallback(rec_option_ui_t *ui, rec_option_done_cb_t on_done, void *user);
void SetRecOptionBackCallback(rec_option_ui_t *ui, rec_option_back_cb_t on_back, void *user);
void GetRecOptionParams(const rec_option_ui_t *ui, rec_option_params_t *out);

}  // namespace gui
}  // namespace core1
