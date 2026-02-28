/**
 * @file ui_bindings.h
 * @brief ScreenStoreが作ったUIへコールバックを接続
 */

#pragma once

// プロジェクトライブラリ
#include "play_agc.h"
#include "screen_store.h"

namespace core1 {
namespace gui {

class UiNavigator;

class UiBindings
{
public:
    UiBindings(ScreenStore &store, UiNavigator &nav) : store_(store), nav_(nav) {}

    void Init();

private:
    static void OnHomeRec(void *user);
    static void OnHomePlay(void *user);
    static void OnBack(void *user);
    static void OnPlayMenu(void *user);
    static void OnRecMenu(void *user);
    static void OnPlayMain(void *user);
    static void OnRecMain(void *user);

    static void OnPlayAgcDoneBridge(const play_agc_params_t *p, void *user);
    static void OnRecOptionDoneBridge(const rec_option_params_t *p, void *user);

    ScreenStore &store_;
    UiNavigator &nav_;
};

}  // namespace gui
}  // namespace core1
