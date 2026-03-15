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

namespace color {

constexpr uint32_t RgbToBgrHex(uint32_t rgb_hex)
{
    return ((rgb_hex & 0xFFU) << 16U) | (rgb_hex & 0x00FF00U) | ((rgb_hex >> 16U) & 0xFFU);
}

inline lv_color_t FromRgb(uint32_t rgb_hex) { return lv_color_hex(RgbToBgrHex(rgb_hex)); }

inline lv_color_t White() { return lv_color_white(); }
inline lv_color_t Black() { return lv_color_black(); }

inline lv_color_t HomeBgBand() { return lv_color_hex(0x2B2B33); }
inline lv_color_t HomeRecPlate() { return lv_color_hex(0xF65C8B); }
inline lv_color_t HomePlayPlate() { return lv_color_hex(0xF6823B); }

inline lv_color_t PlayBg() { return FromRgb(0x0086FF); }
inline lv_color_t RecordBg() { return FromRgb(0x5A5AE6); }
inline lv_color_t CircleButtonDark() { return FromRgb(0x1F1F27); }
inline lv_color_t PlaylistSheetBg() { return FromRgb(0x111827); }
inline lv_color_t PlaylistAccent() { return FromRgb(0x3B82F6); }
inline lv_color_t DoneButtonBg() { return FromRgb(0x6A5ACD); }

}  // namespace color

}  // namespace gui
}  // namespace core1
