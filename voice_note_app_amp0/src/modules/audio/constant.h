#pragma once

#include <cstdint>

namespace core0 {
namespace module {

#define AF_FS   48 /* kHz */
#define AF_MCLK (256 * AF_FS)

static constexpr uint32_t kRun          = 1u << 0;
static constexpr uint32_t kSoftReset    = 1u << 1;
static constexpr uint32_t kErrIrqEn     = 1u << 12;
static constexpr uint32_t kIocIrqEn     = 1u << 13;
static constexpr uint32_t kTimeoutIrqEn = 1u << 14;

static constexpr uint32_t kPcmWidthShift = 16;
static constexpr uint32_t kValidChShift  = 19;

namespace s2mm {

#define AF_S2MM_TIMEOUT 500000000

// オフセット
enum : uint32_t
{
    VERSION         = 0x00,  // MM2Sと共通
    CONFIG          = 0x04,  // MM2Sと共通
    CONTROL         = 0x10,
    STATUS          = 0x14,
    TIMEOUT         = 0x18,
    PERIODS_CONFIG  = 0x1C,
    BUFFER_ADDR_LSB = 0x20,
    BUFFER_ADDR_MSB = 0x24,
    TRANSFER_COUNT  = 0x28,
    CH_STATUS_BITS0 = 0x2C,
    CH_STATUS_BITS1 = 0x30,
    CH_STATUS_BITS2 = 0x34,
    CH_STATUS_BITS3 = 0x38,
    CH_STATUS_BITS4 = 0x3C,
    CH_STATUS_BITS5 = 0x40,
    CH_OFFSET       = 0x44,
};

enum : uint32_t
{
    CTRL_EN_MASK = 0x00000001u,
};
}  // namespace s2mm

namespace mm2s {

// オフセット
enum : uint32_t
{
    CONTROL         = 0x110,
    STATUS          = 0x114,
    MULTIPLIER      = 0x118,
    PERIODS_CONFIG  = 0x11C,
    BUFFER_ADDR_LSB = 0x120,
    BUFFER_ADDR_MSB = 0x124,
    TRANSFER_COUNT  = 0x128,
    CH_STATUS_BITS0 = 0x12C,
    CH_STATUS_BITS1 = 0x130,
    CH_STATUS_BITS2 = 0x134,
    CH_STATUS_BITS3 = 0x138,
    CH_STATUS_BITS4 = 0x13C,
    CH_STATUS_BITS5 = 0x140,
    CH_OFFSET       = 0x144,
};

enum : uint32_t
{
    CTRL_EN_MASK = 0x00000001u,
};
}  // namespace mm2s

}  // namespace module
}  // namespace core0
