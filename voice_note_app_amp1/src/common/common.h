#pragma once

#include <cstdint>

#include "xparameters.h"
#include "xparameters_ps.h"

namespace core1 {
namespace common {

inline constexpr uint32_t kGicBaseAddr    = XPAR_XSCUGIC_0_BASEADDR;  // PS側GICベースアドレス
inline constexpr uint32_t kSpiPsBaseAddr  = XPAR_XSPIPS_0_BASEADDR;   // PS側SPIベースアドレス
inline constexpr uint32_t kGpioPsBaseAddr = XPAR_XGPIOPS_0_BASEADDR;  // PS側GPIOベースアドレス
inline constexpr uint32_t kGpioPlBaseAddr = XPAR_XGPIO_1_BASEADDR;    // PL側GPIOベースアドレス
inline constexpr uint32_t kI2cPlBaseAddr  = XPAR_XIIC_1_BASEADDR;     // PL側I2Cベースアドレス

inline constexpr uint32_t kGpioPlIrqId = XPS_FPGA7_INT_ID;  // PL側GPIO割り込みID
inline constexpr uint32_t kI2cPlIrqId  = XPS_FPGA6_INT_ID;  // PL側I2C割り込みID

}  // namespace common
}  // namespace core1
