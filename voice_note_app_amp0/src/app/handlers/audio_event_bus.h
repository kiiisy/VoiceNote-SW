/**
 * @file audio_event_bus.h
 * @brief System向けイベント受け渡しバス
 */
#pragma once

// プロジェクトライブラリ
#include "fsm.h"

namespace core0 {
namespace app {

/**
 * @brief System内で扱うFSMイベントの一時保持
 *
 * 現段階では「単一の保留イベント」を保持する方式。
 */
class AudioEventBus
{
public:
    void Reset();
    void SetEvent(AudioFsmEvent ev);
    bool Pop(AudioFsmEvent *out);

private:
    bool          has_event_{false};
    AudioFsmEvent event_{AudioFsmEvent::UiPlayPressed};
};

}  // namespace app
}  // namespace core0
