/**
 * @file core_boot.h
 * @brief Core0からCore1を起動するためのブートユーティリティ
 *
 * AMP構成において、Core0からCore1を起動するために
 * CPU1のリセットベクタへ開始アドレスを書き込み、SEVで起床させる
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {
bool BootCpu1(uint32_t start_addr);
}
