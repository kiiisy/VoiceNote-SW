/**
 * @file audio_file_query_handler.h
 * @brief ファイルクエリ系IPCハンドラ
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "app_server.h"

namespace core0 {
namespace app {

class System;

/**
 * @brief ファイル一覧要求を処理する
 */
class AudioFileQueryHandler final
{
public:
    AudioFileQueryHandler(core0::ipc::AppServer &server, System &sys) : server_(server), sys_(sys) {}

    void BindHandlers();

private:
    int32_t OnListDir(const core::ipc::ListDirPayload &p, uint32_t seq);

private:
    static constexpr uint16_t kMaxDirEntriesPerReq = 64;

    core0::ipc::AppServer &server_;
    System           &sys_;
};

}  // namespace app
}  // namespace core0

