/**
 * @file audio_ipc_controller.h
 * @brief AudioIpcControllerの定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "app_server.h"
#include "audio_engine.h"

namespace core0 {
namespace app {

/**
 * @brief IPCとAudioEngineの仲介を担うクラス
 */
class AudioIpcController final
{
public:
    AudioIpcController(core0::ipc::AppServer &server, AudioEngine &sys) : server_(server), sys_(sys) {}

    void SetHandlers();
    void Run();

private:
    int32_t OnPlay(const core::ipc::PlayPayload &p);
    int32_t OnPause();
    int32_t OnResume();
    int32_t OnRecStart(const core::ipc::RecStartPayload &p);
    int32_t OnRecStop();
    int32_t OnListDir(const core::ipc::ListDirPayload &p, uint32_t seq);

    void NotifyStatus();

    void SendPlaybackStatus();
    void SendRecordStatus();
    void SendStatusRegularly();

private:
    static constexpr uint32_t kStatusPeriodMs      = 200;
    static constexpr uint32_t kMaxRecIndex         = 99999;
    static constexpr uint16_t kMaxDirEntriesPerReq = 64;

    uint32_t last_pb_status_ms_{0};   // 再生状態定期送信間隔
    uint32_t last_rec_status_ms_{0};  // 録音状態定期送信間隔
    uint32_t next_rec_index_{1};      // 自動採番用インデックス

    core0::ipc::AppServer &server_;
    AudioEngine             &sys_;
};

}  // namespace app
}  // namespace core0
