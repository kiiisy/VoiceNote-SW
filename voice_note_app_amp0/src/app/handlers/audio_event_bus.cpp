/**
 * @file audio_event_bus.cpp
 * @brief AudioEventBusの実装
 */

// 自ヘッダー
#include "audio_event_bus.h"

namespace core0 {
namespace app {

void AudioEventBus::Reset()
{
    has_event_ = false;
    event_     = AudioFsmEvent::UiPlayPressed;
}

void AudioEventBus::SetEvent(AudioFsmEvent ev)
{
    // 単一スロットなので最新イベントで上書きする。
    has_event_ = true;
    event_     = ev;
}

bool AudioEventBus::Pop(AudioFsmEvent *out)
{
    if (!out || !has_event_) {
        return false;
    }

    *out       = event_;
    has_event_ = false;
    return true;
}

}  // namespace app
}  // namespace core0
