/**
 * @file audio_notification_publisher.cpp
 * @brief AudioNotificationPublisherの実装
 */

// 自ヘッダー
#include "audio_notification_publisher.h"

namespace core0 {
namespace app {

void AudioNotificationPublisher::Reset()
{
    queue_.Reset();
}

void AudioNotificationPublisher::Publish(AudioNotification::Type type, int32_t err)
{
    queue_.Enqueue(type, err);
}

bool AudioNotificationPublisher::Pop(AudioNotification *noti)
{
    return queue_.Dequeue(noti);
}

}  // namespace app
}  // namespace core0
