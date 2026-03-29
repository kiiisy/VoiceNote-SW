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

/**
 * @brief System通知を処理し、必要なIPCステータス送信を実行する
 */
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
                rec_stop_burst_left_ = 0;
                SendRecordStatus();
                break;
            case AudioNotification::Type::kRecordStopped:
                // 停止通知は取りこぼすとUIが復帰できないため、短時間だけ再送をかける
                rec_stop_burst_left_ = kRecordStopBurstCount;
                SendRecordStatus();
                break;

            case AudioNotification::Type::kError:
            default:
                break;
        }
    }

    SendStatusRegularly();
}

/**
 * @brief 再生ステータスをIPCへ送信する
 */
void AudioIpcStatusHandler::SendPlaybackStatus()
{
    core::ipc::PlaybackStatusPayload st{};
    if (sys_.GetStatus(&st)) {
        server_.SendPlaybackStatus(st);
    }
}

/**
 * @brief 録音ステータスをIPCへ送信する
 */
void AudioIpcStatusHandler::SendRecordStatus()
{
    core::ipc::RecordStatusPayload st{};
    if (sys_.GetStatus(&st)) {
        server_.SendRecordStatus(st);
    }
}

/**
 * @brief 再生/録音がアクティブな間、ステータスを定期送信する
 */
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
        return;
    }

    if (rec_stop_burst_left_ > 0u) {
        const uint32_t now = GetTimeMs();
        if ((now - last_rec_status_ms_) >= kStatusPeriodMs) {
            last_rec_status_ms_ = now;
            SendRecordStatus();
            --rec_stop_burst_left_;
        }
    }
}

}  // namespace app
}  // namespace core0
