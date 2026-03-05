/**
 * @file ui_view_updater.h
 * @brief FSMアクションに応じたUI更新を扱うハンドラ
 */

#pragma once

// プロジェクトライブラリ
#include "fsm.h"

namespace core1 {

namespace gui {
class LvglController;
}

namespace app {

/**
 * @brief UI表示系アクションの適用を担当
 */
class UiViewUpdater
{
public:
    void Init(gui::LvglController *gui);
    bool Execute(ActionId action);

private:
    gui::LvglController *gui_{nullptr};
};

}  // namespace app
}  // namespace core1
