#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_format.h"

namespace core0 {

// DMAバッファ設定
inline constexpr uintptr_t kDmaBufBasePhys = 0x10000000u;
inline constexpr uint32_t  kDmaBufBytes    = kBytesPerChunk;  // まずは等倍
inline constexpr uint32_t  kDmaBufAlign    = 64;

// CPU1アドレス
inline constexpr uintptr_t kCpu1StartAddr = 0x08000000u;

}  // namespace core0
