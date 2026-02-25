#pragma once

#include "xil_mmu.h"

/**
 * @brief 指定メモリ領域を非キャッシュ領域として設定する
 *
 * - 1MB単位でアライン調整し、TLB属性を設定する
 *
 * @param base  対象先頭アドレス
 * @param bytes 対象サイズ
 */
void MakeRegionNonCacheable(void *base, size_t bytes)
{
    constexpr uintptr_t kSectionSize = 0x100000;

    uintptr_t start = reinterpret_cast<uintptr_t>(base) & ~(kSectionSize - 1);
    uintptr_t end   = (reinterpret_cast<uintptr_t>(base) + bytes + kSectionSize - 1) & ~(kSectionSize - 1);

    for (uintptr_t p = start; p < end; p += kSectionSize) {
        Xil_SetTlbAttributes(p, NORM_NONCACHE);
    }
}
