/**
 * @file ui_bindings.cpp
 * @brief UiBindingsの実装
 */

// 自ヘッダー
#include "ui_bindings.h"

// LVGLライブラリ
#include "lvgl.h"

// プロジェクトライブラリ
#include "home_screen.h"
#include "logger_core.h"
#include "play_agc_screen.h"
#include "play_screen.h"
#include "record_screen.h"
#include "ui_navigator.h"

namespace core1 {
namespace gui {

void UiBindings::Init()
{
    // ホームのコールバック登録
    SetHomeUiCallback(&store_.GetHomeUi(), &UiBindings::OnHomeRec, &UiBindings::OnHomePlay, this);

    // Recorder callbacks
    SetRecordBackCallback(&store_.GetRecUi(), &UiBindings::OnBack, this);
    SetRecordMainCallback(&store_.GetRecUi(), &UiBindings::OnRecMain, this);

    // Play callbacks
    SetPlayMainCallback(&store_.GetPlayUi(), &UiBindings::OnPlayMain, this);
    SetPlayBackCallback(&store_.GetPlayUi(), &UiBindings::OnBack, this);
    SetPlayMenuCallback(&store_.GetPlayUi(), &UiBindings::OnPlayMenu, this);

    // PlayOptions callbacks
    SetPlayAgcCallback(&store_.GetPlayAgcUi(), &UiBindings::OnPlayAgcDoneBridge, this);

    // ホーム画面のスクロールをOFF
    lv_obj_clear_flag(store_.GetHomeScreen(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(store_.GetHomeScreen(), LV_SCROLLBAR_MODE_OFF);
}

void UiBindings::OnHomeRec(void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self) {
        return;
    }
    self->nav_.GoRec();
}

void UiBindings::OnHomePlay(void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self) {
        return;
    }
    self->nav_.GoPlay();
}

void UiBindings::OnPlayMenu(void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self) {
        return;
    }
    self->nav_.GoPlayOptions();
}

void UiBindings::OnBack(void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self) {
        return;
    }
    self->nav_.Back();
}

void UiBindings::OnPlayMain(void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self) {
        return;
    }
    self->nav_.NotifyPlayMain();
}

void UiBindings::OnRecMain(void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self) {
        return;
    }
    self->nav_.NotifyRecMain();
}

void UiBindings::OnPlayAgcDoneBridge(const play_agc_params_t *p, void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self || !p) {
        return;
    }
    self->nav_.NotifyPlayAgcDone(*p);
}

}  // namespace gui
}  // namespace core1
