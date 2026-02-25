/**
 * @file shared_mm.h
 * @brief OCM周りの定義
 */

#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core {
namespace ipc {

// OCM Highの256KBウィンドウ: 0xFFFC_0000 - 0xFFFF_FFFF
static constexpr uintptr_t kOcmHighBase = 0xFFFC0000u;
static constexpr uintptr_t kOcmHigh1MB  = 0xFFF00000u;  // Xil_SetTlbAttributes用（1MBセクション）

// CPU1のブートベクタ
static constexpr uintptr_t kCpu1Vector = 0xFFFFFFF0u;

// OCM High内の共有領域ベースアドレス（ブートベクタ付近を避ける）
static constexpr uintptr_t kSharedBase = 0xFFFC2000u;

// 共有領域の予約サイズ（リング + ヘッダ）。OCM High 256KB 内に収まること。
static constexpr uint32_t kSharedSize = 96u * 1024u;  // 必要に応じて調整

}  // namespace ipc
}  // namespace core
