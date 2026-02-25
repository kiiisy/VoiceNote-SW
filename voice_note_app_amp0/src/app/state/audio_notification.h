#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {
namespace app {

struct AudioNotification
{
    enum class Type : uint8_t
    {
        kNone = 0,
        kPlaybackStarted,
        kPlaybackStopped,
        kPlaybackPaused,
        kPlaybackResumed,
        kRecordStarted,
        kRecordStopped,
        kError,
    } type = Type::kNone;

    int32_t err = 0;
};

}  // namespace app
}  // namespace core0
