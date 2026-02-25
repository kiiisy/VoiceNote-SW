/**
 * @file fsm.h
 * @brief Fsmの実装
 */

// 自ヘッダー
#include "fsm.h"

// 標準ライブラリ
#include <cstddef>

namespace core1 {
namespace app {

Fsm::Fsm() : st_(State::Idle) {}

Transition Fsm::Dispatch(Event event)
{
    const auto st_index    = static_cast<std::size_t>(st_);
    const auto event_index = static_cast<std::size_t>(event);
    if (st_index >= kStateCount || event_index >= kEventCount) {
        return {st_, ActionId::LogInvalid, ActionId::None};
    }

    const Transition t = kTable[st_index][event_index];
    st_                = t.next;

    return t;
}

}  // namespace app
}  // namespace core1
