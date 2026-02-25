#pragma once

// LVGLライブラリ
#include "lvgl.h"

namespace core1 {
namespace gui {

// RGBからBGRへの変換マクロ
#define LV_COLOR_RGB_AS_BGR(hexRRGGBB)                                                                                 \
    lv_color_hex(((hexRRGGBB & 0xFF) << 16) | (hexRRGGBB & 0x00FF00) | ((hexRRGGBB >> 16) & 0xFF))

/**
 * @brief 指定オブジェクトのスクロール/スクロールバーを無効化する
 *
 * @param obj 対象オブジェクト
 */
inline void KillScroll(lv_obj_t *obj)
{
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

// アニメーションユーティリティ

/**
 * @brief LVGLアニメ exec_cb 用：X座標更新
 * @param obj lv_obj_t* を想定
 * @param v   設定する X 値
 */
inline void SetAnimX(void *obj, int32_t v) { lv_obj_set_x((lv_obj_t *)obj, (lv_coord_t)v); }

/**
 * @brief LVGLアニメ exec_cb 用：Y座標更新
 * @param obj lv_obj_t* を想定
 * @param v   設定する Y 値
 */
inline void SetAnimY(void *obj, int32_t v) { lv_obj_set_y((lv_obj_t *)obj, (lv_coord_t)v); }

}  // namespace gui
}  // namespace core1
