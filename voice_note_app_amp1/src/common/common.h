#pragma once

#include <cstdint>

#include "xparameters.h"
#include "xparameters_ps.h"

namespace core1 {
namespace common {

inline constexpr uint32_t kI2cPlBaseAddr = XPAR_XIIC_1_BASEADDR;  // PL側I2Cベースアドレス
inline constexpr uint32_t kI2cPlIrqId    = XPS_FPGA6_INT_ID;      // PL側I2C割り込みID

}  // namespace common
}  // namespace core1
