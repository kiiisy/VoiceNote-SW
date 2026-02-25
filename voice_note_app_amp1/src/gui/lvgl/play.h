#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core1 {
namespace gui {

// ipc_msg.hのPlayPayloadに合わせる
struct PlayRequest
{
    uint32_t track_id{0};     // 0なら filename 運用でもOK
    char     filename[64]{};  // '\0'終端
};

}  // namespace gui
}  // namespace core1
