/**
 * @file audio_ipc_status_handler.h
 * @brief IPCステータス送信処理ハンドラ
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {
namespace ipc {
class AppServer;
}  // namespace ipc
namespace app {

class System;

/**
 * @brief System通知をIPCステータス送信へ変換する
 */
class AudioIpcStatusHandler final
{
public:
    AudioIpcStatusHandler(ipc::AppServer &server, System &sys) : server_(server), sys_(sys) {}

    void NotifyStatus();

private:
    void SendPlaybackStatus();
    void SendRecordStatus();
    void SendStatusRegularly();

    static constexpr uint32_t kStatusPeriodMs = 200;
    static constexpr uint8_t  kRecordStopBurstCount = 6;

    uint32_t last_pb_status_ms_{0};   // 再生状態定期送信間隔
    uint32_t last_rec_status_ms_{0};  // 録音状態定期送信間隔
    uint8_t  rec_stop_burst_left_{0}; // 録音停止直後の再送回数

    ipc::AppServer &server_;
    System         &sys_;
};

}  // namespace app
}  // namespace core0
