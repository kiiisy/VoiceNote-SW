// 自ヘッダー
#include "app_client.h"

// 標準ライブラリ
#include <cstring>

#include "logger_core.h"

namespace core1 {
namespace ipc {

IpcClient::IpcClient(core::ipc::SharedIpc *s) : s_(s), cmd_tx_(s), evt_rx_(s) {}

// --------------------
// Commands
// --------------------
uint32_t IpcClient::Play(const char *filename)
{
    core::ipc::PlayPayload p{};
    std::memset(&p, 0, sizeof(p));
    p.track_id = 0;
    std::strncpy(p.filename, filename, sizeof(p.filename) - 1);

    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmd(core::ipc::CmdId::Play, seq, p));

    return seq;
}

uint32_t IpcClient::Pause()
{
    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmdRaw(core::ipc::CmdId::Pause, seq, nullptr, 0));

    return seq;
}

uint32_t IpcClient::Resume()
{
    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmdRaw(core::ipc::CmdId::Resume, seq, nullptr, 0));

    return seq;
}

uint32_t IpcClient::Stop()
{
    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmdRaw(core::ipc::CmdId::Stop, seq, nullptr, 0));
    return seq;
}

uint32_t IpcClient::RecStart(const char *filename, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch)
{
    core::ipc::RecStartPayload p{};
    std::memset(&p, 0, sizeof(p));
    std::strncpy(p.filename, filename, sizeof(p.filename) - 1);
    p.sample_rate_hz = sample_rate_hz;
    p.bits           = bits;
    p.ch             = ch;

    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmd(core::ipc::CmdId::RecStart, seq, p));

    return seq;
}

uint32_t IpcClient::RecStop()
{
    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmdRaw(core::ipc::CmdId::RecStop, seq, nullptr, 0));

    return seq;
}

uint32_t IpcClient::ListDir(const char *path)
{
    core::ipc::ListDirPayload p{};
    std::memset(&p, 0, sizeof(p));
    std::strncpy(p.path, path, sizeof(p.path) - 1);

    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmd(core::ipc::CmdId::ListDir, seq, p));

    return seq;
}

uint32_t IpcClient::SetDcCut(bool enable, int32_t fc_q16)
{
    core::ipc::SetDcCutPayload p{};
    std::memset(&p, 0, sizeof(p));
    p.enable = enable ? 1 : 0;
    p.fc_q16 = fc_q16;

    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmd(core::ipc::CmdId::SetDcCut, seq, p));

    return seq;
}

uint32_t IpcClient::SetAgc(bool enable, int16_t dist_mm, int16_t min_gain_x100, int16_t max_gain_x100, int16_t speed_k)
{
    core::ipc::SetAgcPayload p{};
    std::memset(&p, 0, sizeof(p));

    p.enable        = enable ? 1 : 0;
    p.dist_mm       = dist_mm;
    p.min_gain_x100 = min_gain_x100;
    p.max_gain_x100 = max_gain_x100;
    p.speed_k       = speed_k;

    const uint32_t seq = NextSeq();
    cmd_tx_.Send(core::ipc::MakeCmd(core::ipc::CmdId::SetAgc, seq, p));
    return seq;
}

// --------------------
// Poll / Dispatch
// --------------------
void IpcClient::Poll()
{
    core::ipc::Msg ev{};
    while (evt_rx_.Recv(ev)) {  // 「あるだけ捌く」方式
        HandleEvt(ev);
    }
}

void IpcClient::PollN(uint16_t max_msgs)
{
    core::ipc::Msg ev{};
    for (uint16_t i = 0; i < max_msgs; ++i) {
        if (!evt_rx_.Recv(ev)) {
            break;
        }
        HandleEvt(ev);
    }
}

void IpcClient::HandleEvt(const core::ipc::Msg &ev)
{
    const auto id = (core::ipc::EvtId)ev.header.id;

    switch (id) {
        case core::ipc::EvtId::Ack: {
            auto *a = core::ipc::AsPayload<core::ipc::AckPayload>(ev, (uint16_t)core::ipc::EvtId::Ack);
            if (!a) {
                break;
            }
            if (on_ack) {
                on_ack((core::ipc::CmdId)a->req_cmd_id, a->seq, (core::ipc::AckStatus)a->status, a->result_code);
            }
            break;
        }
        case core::ipc::EvtId::Error: {
            auto *e = core::ipc::AsPayload<core::ipc::ErrorPayload>(ev, (uint16_t)core::ipc::EvtId::Error);
            if (!e) {
                break;
            }
            if (on_error) {
                on_error((core::ipc::CmdId)e->req_cmd_id, e->seq, e->error_code, e->detail);
            }
            break;
        }
        case core::ipc::EvtId::ResultChunk: {
            // ResultChunkHdrを見る
            if (ev.header.len < sizeof(core::ipc::ResultChunkHdr)) {
                break;
            }
            auto *rh = reinterpret_cast<const core::ipc::ResultChunkHdr *>(ev.payload);

            if (rh->req_cmd_id == (uint16_t)core::ipc::CmdId::ListDir &&
                rh->kind == (uint16_t)core::ipc::ResultKind::DirEntry &&
                ev.header.len == sizeof(core::ipc::DirEntryPayload)) {

                auto *de = reinterpret_cast<const core::ipc::DirEntryPayload *>(ev.payload);
                if (on_dir_entry)
                    on_dir_entry(de->h.seq, de->h.index, de->h.total_hint, *de);
            }
            break;
        }
        case core::ipc::EvtId::PlaybackStatus: {
            auto *st =
                core::ipc::AsPayload<core::ipc::PlaybackStatusPayload>(ev, (uint16_t)core::ipc::EvtId::PlaybackStatus);
            if (!st) {
                break;
            }
            if (on_playback_status) {
                on_playback_status(*st);
            }
            break;
        }
        case core::ipc::EvtId::RecordStatus: {
            auto *st =
                core::ipc::AsPayload<core::ipc::RecordStatusPayload>(ev, (uint16_t)core::ipc::EvtId::RecordStatus);
            if (!st) {
                break;
            }
            if (on_record_status) {
                on_record_status(*st);
            }
            break;
        }
        default:
            break;
    }
}

}  // namespace ipc
}  // namespace core1
