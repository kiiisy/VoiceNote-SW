/**
 * @file app_event_bus.cpp
 * @brief AppEventBusの実装
 */

// 自ヘッダー
#include "app_event_bus.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core1 {
namespace app {

/**
 * @brief イベントをキューへ追加する
 */
bool AppEventBus::Push(Event event)
{
    if (count_ >= kCapacity) {
        // 固定長キューなので満杯時は破棄してシステム停止を避ける
        LOGW("Event bus overflow: drop event=%u", static_cast<unsigned>(event));
        return false;
    }

    // リングバッファ末尾へ追加
    queue_[tail_] = event;
    tail_         = static_cast<uint8_t>((tail_ + 1U) % kCapacity);
    ++count_;
    return true;
}

/**
 * @brief キュー先頭のイベントを取り出す
 */
bool AppEventBus::Pop(Event *event)
{
    if (!event || count_ == 0U) {
        return false;
    }

    // 先頭を取り出して読み取り位置を1つ進める
    *event = queue_[head_];
    head_  = static_cast<uint8_t>((head_ + 1U) % kCapacity);
    --count_;
    return true;
}

}  // namespace app
}  // namespace core1
