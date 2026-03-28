#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_format.h"

namespace core0 {

// Audio用DDR予約領域
inline constexpr uintptr_t kAudioDdrBasePhys      = 0x10000000u;
inline constexpr uint32_t  kAudioDdrReservedBytes = 6u * 1024u * 1024u;  // 6MB

// DMAワーク領域
inline constexpr uint32_t kAudioDmaWorkBytes = 2u * kBytesPerChunk;

// DDR直再生で使用するバッファ領域
inline constexpr uintptr_t kAudioLinearBasePhys = kAudioDdrBasePhys + kAudioDmaWorkBytes;
inline constexpr uint32_t  kAudioLinearBytes    = kAudioDdrReservedBytes - kAudioDmaWorkBytes;

static_assert(kAudioDdrReservedBytes > kAudioDmaWorkBytes, "Audio DDR layout is invalid");

// 既存コード互換用エイリアス
inline constexpr uintptr_t kDmaBufBasePhys = kAudioDdrBasePhys;
inline constexpr uint32_t  kDmaBufBytes    = kAudioDmaWorkBytes;
inline constexpr uint32_t  kDmaBufAlign    = 64;

// CPU1アドレス
inline constexpr uintptr_t kCpu1StartAddr = 0x08000000u;

}  // namespace core0
