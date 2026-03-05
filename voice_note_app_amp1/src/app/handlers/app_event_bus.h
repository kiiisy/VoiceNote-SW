/**
 * @file app_event_bus.h
 * @brief FSMイベントを一時保持する軽量キュー
 */

#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "fsm.h"

namespace core1 {
namespace app {

/**
 * @brief System内イベント配送用の固定長キュー
 */
class AppEventBus
{
public:
    bool Push(Event event);
    bool Pop(Event *event);

private:
    static constexpr uint8_t kCapacity = 8;
    Event                    queue_[kCapacity]{};
    uint8_t                  head_{0};
    uint8_t                  tail_{0};
    uint8_t                  count_{0};
};

}  // namespace app
}  // namespace core1
