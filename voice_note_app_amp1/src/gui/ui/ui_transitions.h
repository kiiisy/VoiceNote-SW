/**
 * @file ui_transitions.h
 * @brief Home ↔ Rec / Home ↔ Playの演出を担う
 */

#pragma once

// LVGlライブラリ
#include "lvgl.h"

// プロジェクトライブラリ
#include "screen_store.h"

namespace core1 {
namespace gui {

class UiNavigator;

class UiTransitions
{
public:
    UiTransitions(ScreenStore &store, UiNavigator &nav) : store_(store), nav_(nav) {}

    void HomeToRec();
    void HomeToPlay();
    void RecToHome();
    void PlayToHome();

private:
    // アニメ完了系
    static void OnBackToHomeFromPlayAnimDone(lv_anim_t *animation);
    static void OnBackToHomeFromRecAnimDone(lv_anim_t *animation);
    static void OnHomeToPlayAnimDone(lv_anim_t *animation);
    static void OnHomeToRecAnimDone(lv_anim_t *animation);

    struct HomeToRecAnimCtx
    {
        UiTransitions *self{};
        lv_obj_t      *ov_circle{};
        lv_obj_t      *ov_label{};
    };

    struct HomeToPlayAnimCtx
    {
        UiTransitions *self{};
        lv_obj_t      *ov_circle{};
        lv_obj_t      *ov_label{};
    };

    struct BackRecCtx
    {
        UiTransitions *self{};
        lv_obj_t      *moving_scr{};
        lv_obj_t      *ov_label{};
    };

    ScreenStore &store_;
    UiNavigator &nav_;
};

}  // namespace gui
}  // namespace core1
