#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core1 {
namespace gui {

// ipc_msg.h の ListDirPayload に合わせる
struct PlayListRequest
{
    char path[80]{};  // 例: "/"
};

}  // namespace gui
}  // namespace core1
