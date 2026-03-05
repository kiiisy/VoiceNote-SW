/**
 * @file playlist_handler.h
 * @brief ListDir要求と結果集約を扱うハンドラ
 */

#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "ipc_msg.h"
#include "playlist.h"

namespace core1 {

namespace gui {
class LvglController;
}

namespace ipc {
class IpcClient;
}

namespace app {

/**
 * @brief 再生リスト取得の要求/集約/反映を担当
 */
class PlayListHandler
{
public:
    void Init(ipc::IpcClient *ipc, gui::LvglController *gui);
    void Request(const gui::PlayListRequest &req);

private:
    void BindIpcCallbacks();
    void OnDirEntry(uint32_t seq, uint16_t index, uint16_t total_hint, const core::ipc::DirEntryPayload &e);
    void OnAck(core::ipc::CmdId cmd, uint32_t seq, core::ipc::AckStatus st, int32_t rc);

    ipc::IpcClient      *ipc_{nullptr};
    gui::LvglController *gui_{nullptr};

    static constexpr uint16_t kMaxPlayListFiles = 10;
    uint32_t                  list_dir_seq_{0};
    bool                      list_dir_inflight_{false};
    uint16_t                  list_dir_count_{0};
    char                      list_dir_names_[kMaxPlayListFiles][80]{};
    const char               *list_dir_name_ptrs_[kMaxPlayListFiles]{};
};

}  // namespace app
}  // namespace core1
