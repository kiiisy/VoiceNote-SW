/**
 * @file app_server.h
 * @brief IPCサーバ（コマンド受信/イベント送信）
 */
#pragma once

// 標準ライブラリ
#include <cstdint>
#include <functional>

// プロジェクトライブラリ
#include "ipc_msg.h"
#include "mailbox.h"

namespace core0 {
namespace ipc {

/**
 * @brief IPCサーバクラス
 */
class AppServer
{
public:
    explicit AppServer(core::ipc::SharedIpc *ipc);

    std::function<int32_t(const core::ipc::PlayPayload &)>                  on_play;
    std::function<int32_t()>                                                on_stop;
    std::function<int32_t(const core::ipc::RecStartPayload &)>              on_rec_start;
    std::function<int32_t()>                                                on_rec_stop;
    std::function<int32_t(const core::ipc::ListDirPayload &, uint32_t seq)> on_list_dir;
    std::function<int32_t(const core::ipc::RecOptionPayload &)>             on_set_rec_option;
    std::function<int32_t(const core::ipc::SetAgcPayload &)>                on_set_agc;
    std::function<int32_t()>                                                on_pause;
    std::function<int32_t()>                                                on_resume;

    void Poll();
    void Task(uint32_t max_msgs);

    void SendPlaybackStatus(const core::ipc::PlaybackStatusPayload &st);
    void SendRecordStatus(const core::ipc::RecordStatusPayload &st);
    void SendDirEntry(uint32_t seq, uint16_t index, uint16_t total_hint, const char *name, uint32_t size, uint8_t attr);

    void SendAck(core::ipc::CmdId req, uint32_t seq, core::ipc::AckStatus st, int32_t rc = 0);
    void SendError(core::ipc::CmdId req, uint32_t seq, uint16_t code, int32_t detail = 0);

private:
    core::ipc::SharedIpc *ipc_;     // IPC構造体（非所有）
    core::ipc::CmdRx      cmd_rx_;  // コマンド受信
    core::ipc::EvtTx      evt_tx_;  // イベント送信

    void HandleCmd(const core::ipc::Msg &m);

    // 内部エラーコード（あんまり使ってない）
    static constexpr uint16_t kErrUnknownCmd = 1;
    static constexpr uint16_t kErrBadLen     = 2;
};

}  // namespace ipc
}  // namespace core0
