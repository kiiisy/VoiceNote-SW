#pragma once

// 標準ライブラリ
#include <cstdint>
#include <functional>

// プロジェクトライブラリ
#include "ipc_msg.h"
#include "mailbox.h"

namespace core1 {
namespace ipc {

class IpcClient
{
public:
    explicit IpcClient(core::ipc::SharedIpc *s);

    uint32_t Play(const char *filename);
    uint32_t Stop();
    uint32_t RecStart(const char *filename, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch);
    uint32_t RecStop();
    uint32_t ListDir(const char *path);

    uint32_t Pause();
    uint32_t Resume();

    uint32_t SetDcCut(bool enable, int32_t fc_q16);
    uint32_t SetAgc(bool enable, int16_t dist_mm, int16_t min_gain_x100, int16_t max_gain_x100, int16_t speed_k);

    void Poll();               // 1回
    void PollN(uint16_t max_msgs);  // ループ負荷調整用

    std::function<void(core::ipc::CmdId cmd, uint32_t seq, core::ipc::AckStatus st, int32_t rc)> on_ack;
    std::function<void(core::ipc::CmdId cmd, uint32_t seq, uint16_t code, int32_t detail)>       on_error;

    std::function<void(uint32_t seq, uint16_t index, uint16_t total_hint, const core::ipc::DirEntryPayload &e)>
        on_dir_entry;

    std::function<void(const core::ipc::PlaybackStatusPayload &st)> on_playback_status;
    std::function<void(const core::ipc::RecordStatusPayload &st)>   on_record_status;

private:
    core::ipc::SharedIpc *s_;
    core::ipc::CmdTx      cmd_tx_;
    core::ipc::EvtRx      evt_rx_;
    uint32_t              seq_{1};

    uint32_t NextSeq() { return seq_++; }

    void HandleEvt(const core::ipc::Msg &ev);
};

}  // namespace ipc
}  // namespace core1
