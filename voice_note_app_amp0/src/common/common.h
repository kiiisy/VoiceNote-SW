#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xparameters.h"
#include "xparameters_ps.h"

namespace core0 {

// ベースアドレスのリネーム
inline constexpr uint32_t kAudioFormatterBaseAddr = XPAR_XAUDIO_FORMATTER_0_BASEADDR;
inline constexpr uint32_t kPlGpioBaseAddr         = XPAR_XGPIO_0_BASEADDR;
inline constexpr uint32_t kI2cPlBaseAddr          = XPAR_XIIC_0_BASEADDR;
inline constexpr uint32_t kI2sRxBaseAddr          = XPAR_XI2SRX_0_BASEADDR;
inline constexpr uint32_t kI2sTxBaseAddr          = XPAR_XI2STX_0_BASEADDR;
inline constexpr uint32_t kGicBaseAddr            = XPAR_XSCUGIC_0_BASEADDR;
inline constexpr uint32_t PsSpiBaseAddr           = XPAR_XSPIPS_0_BASEADDR;
inline constexpr uint32_t I2sMuxBaseAddr          = XPAR_I2S_CLOCK_MUX_0_BASEADDR;

// 基本フォーマット
constexpr uint32_t kSampleRate   = 48000;  // Hz
constexpr uint32_t kBitDepth     = 16;     // bits
constexpr uint32_t kChannelCount = 2;      // channels

// 1秒あたりのバイト数
constexpr uint32_t kBytesPerSecond = kSampleRate * (kBitDepth / 8) * kChannelCount;  // 192000 bytes/s

// AES（Audio Engine Section）ブロック関連
constexpr uint32_t kFramesPerAESBlock = 192;                                                   // 1 Frame = 192 samples
constexpr uint32_t kAESBlockBytes     = kFramesPerAESBlock * kChannelCount * (kBitDepth / 8);  // 768 bytes

// Audio Formatter (DMA) 設定
constexpr uint32_t kBytesPerPeriod      = kAESBlockBytes * 8;    // 6144 bytes per period
constexpr uint32_t kPeriodsPerChunk     = 16;                    // total 16 periods per chunk
constexpr uint32_t kPeriodsPerHalf      = kPeriodsPerChunk / 2;  // 8 periods per half (A/B)
constexpr uint32_t kPeriodsPerHalfBytes = kBytesPerPeriod * kPeriodsPerHalf;
// 互換名（既存コード向け）
constexpr uint32_t kPeriodsPerCh        = kPeriodsPerHalf;
constexpr uint32_t kPeriodsBytes        = kBytesPerPeriod;

// 1チャンク全体サイズ（16 Periods 分）
constexpr uint32_t kBytesPerChunk = kBytesPerPeriod * kPeriodsPerChunk;  // 98,304 bytes

inline constexpr uintptr_t kDmaBufBasePhys = 0x1000'0000u;
inline constexpr uint32_t  kDmaBufBytes    = kBytesPerChunk;  // まずは等倍
inline constexpr uint32_t  kDmaBufAlign    = 64;

// 割り込み信号のリネーム
inline constexpr uint32_t kAudioFormatterTxIrqId = XPS_FPGA2_INT_ID;
inline constexpr uint32_t kAudioFormatterRxIrqId = XPS_FPGA3_INT_ID;
inline constexpr uint32_t kI2cPlIrqId = XPS_FPGA4_INT_ID;
inline constexpr uint32_t kI2sRxIrqId = XPS_FPGA0_INT_ID;
inline constexpr uint32_t kI2sTxIrqId = XPS_FPGA1_INT_ID;

}  // namespace core0
