#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core1 {
namespace gui {

// ipc_msg.hのPlayPayloadに合わせる
struct PlayAgcRequest
{
    bool    dist_link{false};
    int16_t dist_mm{0};
    int16_t min_gain_x100{50};
    int16_t max_gain_x100{150};
    int16_t speed_k{6};  // 標準のk
};

}  // namespace gui
}  // namespace core1
