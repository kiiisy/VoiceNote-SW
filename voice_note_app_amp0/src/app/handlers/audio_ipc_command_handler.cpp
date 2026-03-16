/**
 * @file audio_ipc_command_handler.cpp
 * @brief AudioIpcCommandHandlerの実装
 */

// 自ヘッダー
#include "audio_ipc_command_handler.h"

// プロジェクトライブラリ
#include "audio_ui_input_handler.h"
#include "logger_core.h"

namespace core0 {
namespace app {

/**
 * @brief IPCコマンドハンドラをAppServerへ登録する
 */
void AudioIpcCommandHandler::BindHandlers()
{
    server_.on_play   = [&](const core::ipc::PlayPayload &p) -> int32_t { return OnPlay(p); };
    server_.on_pause  = [&]() -> int32_t { return OnPause(); };
    server_.on_resume = [&]() -> int32_t { return OnResume(); };

    server_.on_rec_start = [&](const core::ipc::RecStartPayload &p) -> int32_t { return OnRecStart(p); };
    server_.on_rec_stop  = [&]() -> int32_t { return OnRecStop(); };
}

/**
 * @brief 再生開始コマンドを処理する
 *
 * @param[in] p 再生要求ペイロード
 * @retval 0  要求受理
 * @retval -1 要求失敗
 */
int32_t AudioIpcCommandHandler::OnPlay(const core::ipc::PlayPayload &p)
{
    LOGI("Received Play message");
    return ui_input_handler_.RequestPlay(p.filename) ? 0 : -1;
}

/**
 * @brief 一時停止コマンドを処理する
 *
 * @retval 0  要求受理
 * @retval -1 要求失敗
 */
int32_t AudioIpcCommandHandler::OnPause()
{
    LOGI("Received Pause message");
    return ui_input_handler_.RequestPause() ? 0 : -1;
}

/**
 * @brief 再開コマンドを処理する
 *
 * @retval 0  要求受理
 * @retval -1 要求失敗
 */
int32_t AudioIpcCommandHandler::OnResume()
{
    LOGI("Received Resume message");
    return ui_input_handler_.RequestResume() ? 0 : -1;
}

/**
 * @brief 録音開始コマンドを処理する
 *
 * @param[in] p 録音開始要求ペイロード
 * @retval 0  要求受理
 * @retval -1 要求失敗
 */
int32_t AudioIpcCommandHandler::OnRecStart(const core::ipc::RecStartPayload &p)
{
    LOGI("Received Recording start message");
    if (!ui_input_handler_.RequestRecordAuto(p.sample_rate_hz, p.bits, p.ch)) {
        LOGE("RequestRecordAuto failed");
        return -1;
    }
    return 0;
}

/**
 * @brief 録音停止コマンドを処理する
 *
 * @retval 0  要求受理
 * @retval -1 要求失敗
 */
int32_t AudioIpcCommandHandler::OnRecStop()
{
    return ui_input_handler_.RequestRecordStop() ? 0 : -1;
}

}  // namespace app
}  // namespace core0
