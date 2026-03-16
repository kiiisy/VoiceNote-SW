/**
 * @file audio_ipc_command_handler.h
 * @brief IPCコマンド受信処理ハンドラ
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "app_server.h"

namespace core0 {
namespace app {

class AudioUiInputHandler;

/**
 * @brief IPCコマンドをSystem操作へ変換する
 */
class AudioIpcCommandHandler final
{
public:
    AudioIpcCommandHandler(ipc::AppServer &server, AudioUiInputHandler &ui_input_handler)
        : server_(server), ui_input_handler_(ui_input_handler)
    {
    }

    void BindHandlers();

private:
    int32_t OnPlay(const core::ipc::PlayPayload &p);
    int32_t OnPause();
    int32_t OnResume();
    int32_t OnRecStart(const core::ipc::RecStartPayload &p);
    int32_t OnRecStop();

    ipc::AppServer      &server_;
    AudioUiInputHandler &ui_input_handler_;
};

}  // namespace app
}  // namespace core0
