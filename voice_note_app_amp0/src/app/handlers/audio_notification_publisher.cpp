/**
 * @file audio_notification_publisher.cpp
 * @brief AudioNotificationPublisherの実装
 */

// 自ヘッダー
#include "audio_notification_publisher.h"

namespace core0 {
namespace app {

/**
 * @brief 通知キューを初期状態に戻す
 */
void AudioNotificationPublisher::Reset()
{
    queue_.Reset();
}

/**
 * @brief 通知をキューへ発行する
 *
 * @param[in] type 通知種別
 * @param[in] err  エラーコード
 */
void AudioNotificationPublisher::Publish(AudioNotification::Type type, int32_t err)
{
    queue_.Enqueue(type, err);
}

/**
 * @brief 通知キューから1件取り出す
 *
 * @param[out] noti 取り出した通知
 * @retval true  取り出し成功
 * @retval false キュー空または引数不正
 */
bool AudioNotificationPublisher::Pop(AudioNotification *noti)
{
    return queue_.Dequeue(noti);
}

}  // namespace app
}  // namespace core0
