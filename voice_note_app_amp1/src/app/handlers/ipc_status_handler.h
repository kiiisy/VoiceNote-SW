/**
 * @file ipc_status_handler.h
 * @brief IPCステータス受信とイベント化を扱うハンドラ
 */

#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core1 {

namespace gui {
class LvglController;
}

namespace ipc {
class IpcClient;
}

namespace app {

class AppEventBus;

/**
 * @brief IPCステータスをGUI反映しFSMイベントへ変換する
 */
class IpcStatusHandler
{
public:
    void Init(ipc::IpcClient *ipc, gui::LvglController *gui, AppEventBus *event_bus);

private:
    void BindIpcCallbacks();

    ipc::IpcClient      *ipc_{nullptr};
    gui::LvglController *gui_{nullptr};
    AppEventBus         *event_bus_{nullptr};
    uint8_t              last_pb_state_{0xFF};
};

}  // namespace app
}  // namespace core1
