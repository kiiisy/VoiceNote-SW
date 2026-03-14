/**
 * @file audio_file_query_handler.cpp
 * @brief AudioFileQueryHandlerの実装
 */

// 自ヘッダー
#include "audio_file_query_handler.h"

// プロジェクトライブラリ
#include "system.h"
#include "logger_core.h"
#include "wav_reader.h"

namespace core0 {
namespace app {

void AudioFileQueryHandler::BindHandlers()
{
    server_.on_list_dir = [&](const core::ipc::ListDirPayload &p, uint32_t seq) -> int32_t {
        return OnListDir(p, seq);
    };
}

int32_t AudioFileQueryHandler::OnListDir(const core::ipc::ListDirPayload &p, uint32_t seq)
{
    if (sys_.IsPlaybackActive() || sys_.IsRecordActive()) {
        LOGW("ListDir rejected while audio active");
        return -1;
    }

    const char *path = (p.path[0] != '\0') ? p.path : "/";

    uint16_t   sent = 0;
    const bool ok   = module::WavReader::EnumerateWavFiles(
        path, kMaxDirEntriesPerReq, [&](const module::WavReader::WavFileInfo &info) -> bool {
            server_.SendDirEntry(seq, sent, 0, info.path, info.size, 0);
            ++sent;
            return true;
        });

    if (!ok) {
        LOGE("ListDir: EnumerateWavFiles failed path=%s", path);
        return -1;
    }

    LOGI("ListDir: path=%s sent=%u", path, (unsigned)sent);
    return 0;
}

}  // namespace app
}  // namespace core0

