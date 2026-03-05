/**
 * @file ipc_status_handler.cpp
 * @brief IpcStatusHandlerの実装
 */

// 自ヘッダー
#include "ipc_status_handler.h"

// プロジェクトライブラリ
#include "app_client.h"
#include "app_event_bus.h"
#include "ipc_msg.h"
#include "lvgl_controller.h"

namespace core1 {
namespace app {

/**
 * @brief 依存オブジェクトを設定してIPCコールバックを接続する
 */
void IpcStatusHandler::Init(ipc::IpcClient *ipc, gui::LvglController *gui, AppEventBus *event_bus)
{
    ipc_       = ipc;
    gui_       = gui;
    event_bus_ = event_bus;
    BindIpcCallbacks();
}

/**
 * @brief 再生/録音ステータスのIPCコールバックを登録する
 */
void IpcStatusHandler::BindIpcCallbacks()
{
    if (!ipc_) {
        return;
    }

    ipc_->on_playback_status = [this](const core::ipc::PlaybackStatusPayload &st) {
        if (gui_) {
            gui_->SetPlaybackUiState(st.state);
            gui_->SetPlaybackProgress(st.position_ms, st.duration_ms);
        }

        // 停止への遷移だけイベント化する
        const uint8_t prev = last_pb_state_;
        last_pb_state_     = st.state;

        if (event_bus_ && prev != st.state && st.state == 0U) {
            event_bus_->Push(Event::IpcPlaybackStopped);
        }
    };

    ipc_->on_record_status = [this](const core::ipc::RecordStatusPayload &st) {
        if (gui_) {
            gui_->SetRecordUiState(st.state);
            gui_->SetRecordProgress(st.captured_ms, st.target_ms);
        }

        // 録音停止通知は毎回受けるので、state==0を直接イベント化する
        if (event_bus_ && st.state == 0U) {
            event_bus_->Push(Event::IpcRecordStopped);
        }
    };
}

}  // namespace app
}  // namespace core1
