#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_notification.h"

namespace core0 {
namespace app {

/**
 * @brief AudioNotificationのキュークラス
 */
class AudioNotificationQueue final
{
public:
    void Enqueue(AudioNotification::Type type, int32_t err)
    {
        noti_.type = type;
        noti_.err  = err;
        pending_   = true;
    }

    bool Dequeue(AudioNotification *noti)
    {
        if (!pending_ || !noti) {
            return false;
        }

        *noti    = noti_;
        pending_ = false;

        return true;
    }

    void Reset()
    {
        pending_ = false;
        noti_    = AudioNotification{};
    }

private:
    bool              pending_{false};
    AudioNotification noti_{};
};

}  // namespace app
}  // namespace core0
