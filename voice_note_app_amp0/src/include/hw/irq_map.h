#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xparameters.h"

namespace core0 {

// 割り込み信号のリネーム
inline constexpr uint32_t kAudioFormatterTxIrqId = XPS_FPGA2_INT_ID;
inline constexpr uint32_t kAudioFormatterRxIrqId = XPS_FPGA3_INT_ID;
inline constexpr uint32_t kI2cPlIrqId            = XPS_FPGA4_INT_ID;
inline constexpr uint32_t kI2sRxIrqId            = XPS_FPGA0_INT_ID;
inline constexpr uint32_t kI2sTxIrqId            = XPS_FPGA1_INT_ID;

}  // namespace core0
