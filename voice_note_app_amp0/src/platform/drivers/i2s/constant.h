#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "common.h"

namespace i2stx {

static_assert((core0::kSampleRate % 1000u) == 0u, "kSampleRate must be divisible by 1000 for *_FS_KHZ.");

inline constexpr uint32_t kI2sTxfSKhz = core0::kSampleRate / 1000u;
inline constexpr uint32_t kI2sTxMclk  = 256u * kI2sTxfSKhz;
inline constexpr uint32_t kI2sRxFsKhz = core0::kSampleRate / 1000u;
inline constexpr uint32_t kI2sRxMclk  = 256u * kI2sRxFsKhz;

// オフセット
enum : uint32_t
{
    VERSION      = 0x00,
    CONFIG       = 0x04,
    CONTROL      = 0x08,
    VALIDITY     = 0x0C,
    IRQ_ENABLE   = 0x10,
    IRQ_STATUS   = 0x14,
    SCLK_DIV     = 0x20,
    CH_CTRL_01   = 0x30,
    CH_CTRL_23   = 0x34,
    CH_CTRL_45   = 0x38,
    CH_CTRL_67   = 0x3C,
    AES_STATUS_0 = 0x50,
    AES_STATUS_1 = 0x54,
    AES_STATUS_2 = 0x58,
    AES_STATUS_3 = 0x5C,
    AES_STATUS_4 = 0x60,
    AES_STATUS_5 = 0x64,
};

enum : uint32_t
{
    MUX_CONTROL = 0x00,
};

enum : uint32_t
{
    CTRL_EN_MASK = 0x00000001u,
};

}  // namespace i2stx
