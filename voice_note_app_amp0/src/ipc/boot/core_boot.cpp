/**
 * @file core_boot.cpp
 * @brief core_bootの実装
 */

// 自ヘッダー
#include "core_boot.h"

// Vitisライブラリ
#include "xil_cache.h"
#include "xil_io.h"
#include "xpseudo_asm_gcc.h"

// プロジェクトライブラリ
#include "amp_hw.h"
#include "shared_mm.h"

namespace {

void WriteCpu1BootVector(uint32_t start_addr)
{
    Xil_Out32((INTPTR)core::ipc::kCpu1Vector, start_addr);

    // 他コア可視化のために該当アドレスをflush
    Xil_DCacheFlushRange((INTPTR)core::ipc::kCpu1Vector, 4);

    // flush 完了順序を確定
    dsb();
}

}  // namespace

namespace core0 {

/**
 * @brief Core1を指定アドレスから起動する
 *
 * @param start_addr Core1の開始アドレス（32-bitアライン必須）
 * @return 起動手順の実行可否（アラインチェックのみ）
 */
bool BootCpu1(uint32_t start_addr)
{
    // A9命令フェッチの前提として32-bitアラインのみ許可
    if (start_addr & 0x3u) {
        return false;
    }

    // CPU1のブートベクタへ開始アドレスを書き込む
    WriteCpu1BootVector(start_addr);

    // ベクタ書き込みを可視化してからイベントを発行する
    dsb();

    // SEVでWFE(Wait For Entry)待機中のCPU1を起床させる
    __asm__("sev");

    // 起床イベント発行後の順序を確定
    dsb();

    return true;
}

}  // namespace core0
