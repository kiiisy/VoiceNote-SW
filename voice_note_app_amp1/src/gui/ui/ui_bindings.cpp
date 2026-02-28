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

    // RECのコールバック登録
    SetRecordBackCallback(&store_.GetRecUi(), &UiBindings::OnBack, this);
    SetRecordMenuCallback(&store_.GetRecUi(), &UiBindings::OnRecMenu, this);
    SetRecordMainCallback(&store_.GetRecUi(), &UiBindings::OnRecMain, this);

    // Playのコールバック登録
    SetPlayMainCallback(&store_.GetPlayUi(), &UiBindings::OnPlayMain, this);
    SetPlayBackCallback(&store_.GetPlayUi(), &UiBindings::OnBack, this);
    SetPlayMenuCallback(&store_.GetPlayUi(), &UiBindings::OnPlayMenu, this);

    // Optionsのコールバック登録
    SetPlayAgcCallback(&store_.GetPlayAgcUi(), &UiBindings::OnPlayAgcDoneBridge, this);
    SetRecOptionCallback(&store_.GetRecOptionUi(), &UiBindings::OnRecOptionDoneBridge, this);

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

void UiBindings::OnRecMenu(void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self) {
        return;
    }
    self->nav_.GoRecOptions();
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

void UiBindings::OnRecOptionDoneBridge(const rec_option_params_t *p, void *user)
{
    auto *self = static_cast<UiBindings *>(user);
    if (!self || !p) {
        return;
    }
    self->nav_.NotifyRecOptionDone(*p);
}

}  // namespace gui
}  // namespace core1
