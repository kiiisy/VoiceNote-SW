/**
 * @file ui_view_updater.cpp
 * @brief UiViewUpdaterの実装
 */

// 自ヘッダー
#include "ui_view_updater.h"

// プロジェクトライブラリ
#include "lvgl_controller.h"

namespace core1 {
namespace app {

/**
 * @brief 依存GUIオブジェクトを設定する
 */
void UiViewUpdater::Init(gui::LvglController *gui) { gui_ = gui; }

/**
 * @brief UI表示系アクションを適用する
 */
bool UiViewUpdater::Execute(ActionId action)
{
    switch (action) {
        case ActionId::UiPlayShowPlayIcon:
        case ActionId::UiPlayShowPauseIcon:
            return true;

        case ActionId::UiRecShowPlayIcon:
            if (gui_) {
                gui_->SetRecordView(false);
            }
            return true;

        case ActionId::UiRecShowStopIcon:
            if (gui_) {
                gui_->SetRecordView(true);
            }
            return true;

        default:
            return false;
    }
}

}  // namespace app
}  // namespace core1
