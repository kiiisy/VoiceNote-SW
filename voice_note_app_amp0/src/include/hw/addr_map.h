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
inline constexpr uint32_t kAgcBaseAddr            = XPAR_AGC_0_BASEADDR;
inline constexpr uint32_t kAcuBaseAddr            = XPAR_XAUDIO_CLEAN_UP_0_BASEADDR;
inline constexpr uint32_t kArecBaseAddr           = XPAR_AREC_0_BASEADDR;
inline constexpr uint32_t kGicBaseAddr            = XPAR_XSCUGIC_0_BASEADDR;
inline constexpr uint32_t kPsSpiBaseAddr          = XPAR_XSPIPS_0_BASEADDR;
inline constexpr uint32_t kI2sMuxBaseAddr         = XPAR_I2S_CLOCK_MUX_0_BASEADDR;

}  // namespace core0
