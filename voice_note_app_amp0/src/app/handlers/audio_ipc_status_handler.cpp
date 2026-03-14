/**
 * @file audio_ipc_status_handler.cpp
 * @brief AudioIpcStatusHandlerの実装
 */

// 自ヘッダー
#include "audio_ipc_status_handler.h"

// プロジェクトライブラリ
#include "app_server.h"
#include "system.h"
#include "audio_notification.h"
#include "utility.h"

namespace core0 {
namespace app {

void AudioIpcStatusHandler::NotifyStatus()
{
    AudioNotification noti;
    while (sys_.GetNextNotification(&noti)) {
        switch (noti.type) {
            case AudioNotification::Type::kPlaybackStarted:
            case AudioNotification::Type::kPlaybackPaused:
            case AudioNotification::Type::kPlaybackResumed:
            case AudioNotification::Type::kPlaybackStopped:
                SendPlaybackStatus();
                break;

            case AudioNotification::Type::kRecordStarted:
            case AudioNotification::Type::kRecordStopped:
                SendRecordStatus();
                break;

            case AudioNotification::Type::kError:
            default:
                break;
        }
    }

    SendStatusRegularly();
}

void AudioIpcStatusHandler::SendPlaybackStatus()
{
    core::ipc::PlaybackStatusPayload st{};
    if (sys_.GetStatus(&st)) {
        server_.SendPlaybackStatus(st);
    }
}

void AudioIpcStatusHandler::SendRecordStatus()
{
    core::ipc::RecordStatusPayload st{};
    if (sys_.GetStatus(&st)) {
        server_.SendRecordStatus(st);
    }
}

void AudioIpcStatusHandler::SendStatusRegularly()
{
    if (sys_.IsPlaybackActive()) {
        const uint32_t now = GetTimeMs();
        if ((now - last_pb_status_ms_) < kStatusPeriodMs) {
            // 周期未満でも送信する現行挙動を維持する
        }
        last_pb_status_ms_ = now;
        SendPlaybackStatus();
    }

    if (sys_.IsRecordActive()) {
        const uint32_t now = GetTimeMs();
        if ((now - last_rec_status_ms_) < kStatusPeriodMs) {
            // 周期未満でも送信する現行挙動を維持する
        }
        last_rec_status_ms_ = now;
        SendRecordStatus();
    }
}

}  // namespace app
}  // namespace core0
