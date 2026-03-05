/**
 * @file ipc_command_handler.h
 * @brief FSMアクションをIPCコマンドへ変換するハンドラ
 */

#pragma once

// プロジェクトライブラリ
#include "fsm.h"

namespace core1 {

namespace ipc {
class IpcClient;
}

namespace app {

class UiInputHandler;

/**
 * @brief 再生/録音のコマンド送信を担当
 */
class IpcCommandHandler
{
public:
    void Init(ipc::IpcClient *ipc, const UiInputHandler *ui_input_handler);
    bool Execute(ActionId action);

private:
    ipc::IpcClient       *ipc_{nullptr};
    const UiInputHandler *ui_input_handler_{nullptr};
};

}  // namespace app
}  // namespace core1
