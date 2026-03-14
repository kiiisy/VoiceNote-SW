/**
 * @file audio_notification_publisher.h
 * @brief Audio通知発行ハンドラ
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_notification.h"

namespace core0 {
namespace app {

/**
 * @brief Audio通知キューへの発行と取り出しを担当する
 */
class AudioNotificationPublisher final
{
public:
    void Reset();
    void Publish(AudioNotification::Type type, int32_t err);
    bool Pop(AudioNotification *noti);

private:
    AudioNotificationQueue queue_{};
};

}  // namespace app
}  // namespace core0
