/**
 * @file app_server.cpp
 * @brief app_serverの実装
 */

// 自ヘッダー
#include "app_server.h"

// 標準ライブラリ
#include <cstring>

#include "logger_core.h"

namespace core0 {
namespace ipc {

AppServer::AppServer(core::ipc::SharedIpc *ipc) : ipc_(ipc), cmd_rx_(ipc), evt_tx_(ipc) {}

/**
 * @brief 受信できるだけコマンドを処理する
 */
void AppServer::Poll()
{
    core::ipc::Msg msg{};
    while (cmd_rx_.Recv(msg)) {
        HandleCmd(msg);
    }
}

/**
 * @brief 最大max_msgs件だけコマンドを処理する
 *
 * @param max_msgs 1回の呼び出しで処理する上限数
 */
void AppServer::Task(uint32_t max_msgs)
{
    core::ipc::Msg msg{};
    for (uint32_t i = 0; i < max_msgs; ++i) {
        if (!cmd_rx_.Recv(msg)) {
            break;
        }
        HandleCmd(msg);
    }
}

/**
 * @brief Ack イベントを送信する
 *
 * @param req 要求コマンドID
 * @param seq 要求のシーケンス番号
 * @param st  Ack ステータス
 * @param rc  処理結果コード（0=成功, 負値=失敗）
 */
void AppServer::SendAck(core::ipc::CmdId req, uint32_t seq, core::ipc::AckStatus st, int32_t rc)
{
    core::ipc::AckPayload payload{};

    payload.req_cmd_id  = (uint16_t)req;
    payload.status      = (uint16_t)st;
    payload.seq         = seq;
    payload.result_code = rc;
    evt_tx_.Send(core::ipc::MakeEvt(core::ipc::EvtId::Ack, seq, payload));
}

/**
 * @brief Error イベントを送信する
 *
 * @param req    要求コマンドID
 * @param seq    要求のシーケンス番号
 * @param code   エラーコード（プロトコル/アプリで定義）
 * @param detail 追加情報（負の rc など）
 */
void AppServer::SendError(core::ipc::CmdId req, uint32_t seq, uint16_t code, int32_t detail)
{
    core::ipc::ErrorPayload payload{};

    payload.req_cmd_id = (uint16_t)req;
    payload.error_code = code;
    payload.seq        = seq;
    payload.detail     = detail;
    evt_tx_.Send(core::ipc::MakeEvt(core::ipc::EvtId::Error, seq, payload));
}

/**
 * @brief 再生状態の自発通知を送信する
 *
 * @param st 再生状態ペイロード
 */
void AppServer::SendPlaybackStatus(const core::ipc::PlaybackStatusPayload &st)
{
    evt_tx_.Send(core::ipc::MakeEvt(core::ipc::EvtId::PlaybackStatus, 0, st));
}

void AppServer::SendRecordStatus(const core::ipc::RecordStatusPayload &st)
{
    evt_tx_.Send(core::ipc::MakeEvt(core::ipc::EvtId::RecordStatus, 0, st));
}

/**
 * @brief ListDirの1件分の結果を送信する
 *
 * @param seq        要求のシーケンス番号
 * @param index      何番目の要素か
 * @param total_hint 総数のヒント（不明なら0でも可）
 * @param name       ファイル名（NULL可）
 * @param size       ファイルサイズ（バイト）
 * @param attr       属性（FatFs attr 等）
 */
void AppServer::SendDirEntry(uint32_t seq, uint16_t index, uint16_t total_hint, const char *name, uint32_t size,
                             uint8_t attr)
{
    core::ipc::DirEntryPayload payload{};

    payload.h.req_cmd_id = (uint16_t)core::ipc::CmdId::ListDir;
    payload.h.kind       = (uint16_t)core::ipc::ResultKind::DirEntry;
    payload.h.seq        = seq;
    payload.h.index      = index;
    payload.h.total_hint = total_hint;

    payload.size = size;
    payload.attr = attr;
    std::memset(payload.name, 0, sizeof(payload.name));
    if (name) {
        std::strncpy(payload.name, name, sizeof(payload.name) - 1);
    }

    evt_tx_.Send(core::ipc::MakeEvt(core::ipc::EvtId::ResultChunk, seq, payload));
}

/**
 * @brief 受信コマンドを解釈してハンドラを呼び、応答イベントを返す
 *
 * @param msg 受信メッセージ
 */
void AppServer::HandleCmd(const core::ipc::Msg &msg)
{
    const auto cmd = (core::ipc::CmdId)msg.header.id;

    // 受理通知（消してもいいかも）
    SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Accepted, 0);

    LOGI("Received Cmd : %d", cmd);

    switch (cmd) {
        case core::ipc::CmdId::Play: {
            auto *p = core::ipc::AsPayload<core::ipc::PlayPayload>(msg, (uint16_t)core::ipc::CmdId::Play);
            if (!p) {
                SendError(cmd, msg.header.seq, kErrBadLen);
                break;
            }

            int32_t rc = on_play ? on_play(*p) : 0;
            if (rc < 0) {
                SendError(cmd, msg.header.seq, /*code*/ 100, rc);
            }

            SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Done, rc);

            break;
        }

        case core::ipc::CmdId::Pause: {
            if (msg.header.len != 0) {
                // とりあえずなにもしない
            }

            int32_t rc = on_pause ? on_pause() : 0;
            if (rc < 0) {
                SendError(cmd, msg.header.seq, 106, rc);
            }

            SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Done, rc);

            break;
        }

        case core::ipc::CmdId::Resume: {
            if (msg.header.len != 0) {
                // とりあえずなにもしない
            }

            int32_t rc = on_resume ? on_resume() : 0;
            if (rc < 0) {
                SendError(cmd, msg.header.seq, 107, rc);
            }

            SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Done, rc);

            break;
        }

        case core::ipc::CmdId::RecStart: {
            auto *p = core::ipc::AsPayload<core::ipc::RecStartPayload>(msg, (uint16_t)core::ipc::CmdId::RecStart);
            if (!p) {
                SendError(cmd, msg.header.seq, kErrBadLen);
                break;
            }

            int32_t rc = on_rec_start ? on_rec_start(*p) : 0;
            if (rc < 0) {
                SendError(cmd, msg.header.seq, 102, rc);
            }

            SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Done, rc);

            break;
        }

        case core::ipc::CmdId::RecStop: {
            int32_t rc = on_rec_stop ? on_rec_stop() : 0;
            if (rc < 0) {
                SendError(cmd, msg.header.seq, 103, rc);
            }

            SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Done, rc);

            break;
        }

        case core::ipc::CmdId::SetRecOption: {
            auto *p =
                core::ipc::AsPayload<core::ipc::RecOptionPayload>(msg, (uint16_t)core::ipc::CmdId::SetRecOption);
            if (!p) {
                SendError(cmd, msg.header.seq, kErrBadLen);
                break;
            }

            int32_t rc = on_set_rec_option ? on_set_rec_option(*p) : 0;
            if (rc < 0) {
                SendError(cmd, msg.header.seq, 104, rc);
            }

            SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Done, rc);

            break;
        }

        case core::ipc::CmdId::ListDir: {
            auto *p = core::ipc::AsPayload<core::ipc::ListDirPayload>(msg, (uint16_t)core::ipc::CmdId::ListDir);
            if (!p) {
                SendError(cmd, msg.header.seq, kErrBadLen);
                break;
            }

            int32_t rc = on_list_dir ? on_list_dir(*p, msg.header.seq) : 0;
            if (rc < 0) {
                SendError(cmd, msg.header.seq, 105, rc);
            }

            SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Done, rc);

            break;
        }

        case core::ipc::CmdId::SetAgc: {
            auto *p = core::ipc::AsPayload<core::ipc::SetAgcPayload>(msg, (uint16_t)core::ipc::CmdId::SetAgc);
            if (!p) {
                SendError(cmd, msg.header.seq, kErrBadLen);
                break;
            }

            int32_t rc = on_set_agc ? on_set_agc(*p) : 0;
            if (rc < 0) {
                SendError(cmd, msg.header.seq, 108, rc);
            }

            SendAck(cmd, msg.header.seq, core::ipc::AckStatus::Done, rc);

            break;
        }

        default:
            SendError(cmd, msg.header.seq, kErrUnknownCmd);
            break;
    }
}

}  // namespace ipc
}  // namespace core0
