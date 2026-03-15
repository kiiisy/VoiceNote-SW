/**
 * @file playlist_handler.cpp
 * @brief PlayListHandlerの実装
 */

// 自ヘッダー
#include "playlist_handler.h"

// 標準ライブラリ
#include <cstring>

// プロジェクトライブラリ
#include "app_client.h"
#include "lvgl_controller.h"

namespace core1 {
namespace app {

/**
 * @brief 依存オブジェクトを設定してコールバックを接続する
 */
void PlayListHandler::Init(ipc::IpcClient *ipc, gui::LvglController *gui)
{
    ipc_ = ipc;
    gui_ = gui;

    for (uint16_t i = 0; i < kMaxPlayListFiles; ++i) {
        list_dir_names_[i][0]  = '\0';
        list_dir_name_ptrs_[i] = list_dir_names_[i];
    }

    BindIpcCallbacks();
}

/**
 * @brief GUIからの再生リスト要求をIPCへ送る
 */
void PlayListHandler::Request(const gui::PlayListRequest &req)
{
    if (!ipc_) {
        return;
    }

    list_dir_count_    = 0;
    list_dir_inflight_ = true;

    for (uint16_t i = 0; i < kMaxPlayListFiles; ++i) {
        list_dir_names_[i][0]  = '\0';
        list_dir_name_ptrs_[i] = list_dir_names_[i];
    }

    list_dir_seq_ = ipc_->ListDir(req.path);
}

/**
 * @brief ListDir関連のIPCコールバックを登録する
 */
void PlayListHandler::BindIpcCallbacks()
{
    if (!ipc_) {
        return;
    }

    // 既存ハンドラがいれば先に呼んで、既存処理との共存を維持する
    auto prev_on_dir_entry = ipc_->on_dir_entry;
    ipc_->on_dir_entry     = [this, prev_on_dir_entry](uint32_t seq, uint16_t index, uint16_t total_hint,
                                                   const core::ipc::DirEntryPayload &e) {
        if (prev_on_dir_entry) {
            prev_on_dir_entry(seq, index, total_hint, e);
        }
        OnDirEntry(seq, index, total_hint, e);
    };

    auto prev_on_ack = ipc_->on_ack;
    ipc_->on_ack     = [this, prev_on_ack](core::ipc::CmdId cmd, uint32_t seq, core::ipc::AckStatus st, int32_t rc) {
        if (prev_on_ack) {
            prev_on_ack(cmd, seq, st, rc);
        }
        OnAck(cmd, seq, st, rc);
    };
}

/**
 * @brief DirEntryを受信して一覧へ蓄積する
 */
void PlayListHandler::OnDirEntry(uint32_t seq, uint16_t index, uint16_t total_hint, const core::ipc::DirEntryPayload &e)
{
    (void)index;
    (void)total_hint;

    // 今回要求したListDirの応答だけを受け付ける
    if (!list_dir_inflight_ || seq != list_dir_seq_) {
        return;
    }
    if (list_dir_count_ >= kMaxPlayListFiles) {
        return;
    }
    if (e.attr != 0) {
        return;
    }

    std::strncpy(list_dir_names_[list_dir_count_], e.name, sizeof(list_dir_names_[list_dir_count_]) - 1U);
    list_dir_names_[list_dir_count_][sizeof(list_dir_names_[list_dir_count_]) - 1U] = '\0';

    ++list_dir_count_;
}

/**
 * @brief ListDir完了ACKを受信してUIへ反映する
 */
void PlayListHandler::OnAck(core::ipc::CmdId cmd, uint32_t seq, core::ipc::AckStatus st, int32_t rc)
{
    // ListDir完了ACK以外は無視する
    if (cmd != core::ipc::CmdId::ListDir || seq != list_dir_seq_ || st != core::ipc::AckStatus::Done) {
        return;
    }

    list_dir_inflight_ = false;
    if (rc < 0) {
        // エラー時は一覧を空としてUIへ反映する
        list_dir_count_ = 0;
    }

    for (uint16_t i = 0; i < kMaxPlayListFiles; ++i) {
        list_dir_name_ptrs_[i] = list_dir_names_[i];
    }

    if (gui_) {
        gui_->SetPlayFileList(list_dir_name_ptrs_, list_dir_count_);
    }
}

}  // namespace app
}  // namespace core1
