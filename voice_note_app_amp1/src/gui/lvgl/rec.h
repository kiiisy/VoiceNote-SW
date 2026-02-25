#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core1 {
namespace gui {

// ipc_msg.hのPlayPayloadに合わせる
struct RecRequest
{
    char     filename[64];    // ファイル名
    uint32_t sample_rate_hz;  // サンプリング
    uint16_t bits;            // 0..1000 etc
    uint16_t ch;              // 予約
};

}  // namespace gui
}  // namespace core1
