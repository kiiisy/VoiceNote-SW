#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {

// 基本フォーマット
inline constexpr uint32_t kSampleRate   = 48000;  // Hz
inline constexpr uint32_t kBitDepth     = 16;     // bits
inline constexpr uint32_t kChannelCount = 2;      // channels

// 1秒あたりのバイト数
inline constexpr uint32_t kBytesPerSecond = kSampleRate * (kBitDepth / 8) * kChannelCount;  // 192000 bytes/s

// AES（Audio Engine Section）ブロック関連
inline constexpr uint32_t kFramesPerAESBlock = 192;  // 1 Frame = 192 samples
inline constexpr uint32_t kAESBlockBytes     = kFramesPerAESBlock * kChannelCount * (kBitDepth / 8);  // 768 bytes

// Audio Formatter (DMA) 設定
inline constexpr uint32_t kBytesPerPeriod      = kAESBlockBytes * 8;    // 6144 bytes per period
inline constexpr uint32_t kPeriodsPerChunk     = 16;                    // total 16 periods per chunk
inline constexpr uint32_t kPeriodsPerHalf      = kPeriodsPerChunk / 2;  // 8 periods per half (A/B)
inline constexpr uint32_t kPeriodsPerHalfBytes = kBytesPerPeriod * kPeriodsPerHalf;

// 1チャンク全体サイズ（16Periods分）
inline constexpr uint32_t kBytesPerChunk = kBytesPerPeriod * kPeriodsPerChunk;  // 98,304 bytes

}  // namespace core0
