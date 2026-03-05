/**
 * @file ipc_command_handler.cpp
 * @brief IpcCommandHandlerの実装
 */

// 自ヘッダー
#include "ipc_command_handler.h"

// プロジェクトライブラリ
#include "app_client.h"
#include "logger_core.h"
#include "ui_input_handler.h"

namespace core1 {
namespace app {

/**
 * @brief 依存オブジェクトを設定する
 */
void IpcCommandHandler::Init(ipc::IpcClient *ipc, const UiInputHandler *ui_input_handler)
{
    ipc_              = ipc;
    ui_input_handler_ = ui_input_handler;
}

/**
 * @brief FSMアクションをIPCコマンドへ変換して実行する
 */
bool IpcCommandHandler::Execute(ActionId action)
{
    switch (action) {
        case ActionId::SendPlay:
            if (!ipc_ || !ui_input_handler_) {
                return true;
            }
            // Playは最新UI要求を参照して実行する
            if (!ui_input_handler_->HasPlayRequest()) {
                LOGW("SendPlay without cached PlayRequest");
                return true;
            }
            LOGI("Play: track_id=%u filename=%s", static_cast<unsigned>(ui_input_handler_->GetPlayRequest().track_id),
                 ui_input_handler_->GetPlayRequest().filename);
            ipc_->Play(ui_input_handler_->GetPlayRequest().filename);
            return true;

        case ActionId::SendPause:
            if (ipc_) {
                ipc_->Pause();
            }
            return true;

        case ActionId::SendResume:
            if (ipc_) {
                ipc_->Resume();
            }
            return true;

        case ActionId::SendRecStart:
            if (!ipc_ || !ui_input_handler_) {
                return true;
            }
            // RecStartも最新UI要求を参照して実行する
            if (!ui_input_handler_->HasRecRequest()) {
                LOGW("SendRecStart without cached RecordRequest");
                return true;
            }
            ipc_->RecStart(ui_input_handler_->GetRecRequest().filename,
                           ui_input_handler_->GetRecRequest().sample_rate_hz, ui_input_handler_->GetRecRequest().bits,
                           ui_input_handler_->GetRecRequest().ch);
            return true;

        case ActionId::SendRecStop:
            if (ipc_) {
                ipc_->RecStop();
            }
            return true;

        default:
            return false;
    }
}

}  // namespace app
}  // namespace core1
