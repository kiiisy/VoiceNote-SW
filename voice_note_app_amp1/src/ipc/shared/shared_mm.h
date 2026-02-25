#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core {
namespace ipc {

// OCM High 256KB window: 0xFFFC_0000 - 0xFFFF_FFFF
static constexpr uintptr_t kOcmHighBase = 0xFFFC0000u;
static constexpr uintptr_t kOcmHigh1MB  = 0xFFF00000u;  // for Xil_SetTlbAttributes (1MB section)

// CPU1 boot vector read by park code
static constexpr uintptr_t kCpu1Vector = 0xFFFFFFF0u;

// Shared region base within OCM high. Keep away from the top where the boot vector lives.
static constexpr uintptr_t kSharedBase = 0xFFFC2000u;

// Reserve size for shared (rings + header). Must fit within OCM high 256KB.
static constexpr uint32_t kSharedSize = 96u * 1024u;  // adjust if needed

}  // namespace ipc
}  // namespace core
