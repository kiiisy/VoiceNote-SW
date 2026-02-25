/**
 * @file amp_hw.cpp
 * @brief amp_hwの実装
 */

// 自ヘッダー
#include "amp_hw.h"

// Vitisライブラリ
#include "xil_io.h"
#include "xil_mmu.h"
#include "xpseudo_asm_gcc.h"

// プロジェクトライブラリ
#include "shared_mm.h"

namespace {

constexpr uintptr_t SLCR_UNLOCK  = 0xF8000008u;  // SLCR unlockレジスタアドレス
constexpr uintptr_t SLCR_LOCK    = 0xF8000004u;  // SLCR lockレジスタアドレス
constexpr uintptr_t SLCR_OCM_CFG = 0xF8000910u;  // OCM high windowレジスタ

/**
 * @brief OCM High windowを256KB(64KB x4)全域有効化する
 */
void EnableOcmHigh()
{
    // SLCR unlock
    Xil_Out32(SLCR_UNLOCK, 0x0000DF0Du);
    dmb();

    // OCM High window: 64KB x4 = 256KB
    Xil_Out32(SLCR_OCM_CFG, 0x0000000Fu);
    dmb();

    // SLCR lock
    Xil_Out32(SLCR_LOCK, 0x0000767Bu);
    dmb();
}

/**
 * @brief OCM Highを含む1MBセクションを非キャッシュに設定する
 */
void MakeOcmHighNonCache()
{
    // OCM high (0xFFFC0000-0xFFFFFFFF) を含む最小1MBは 0xFFF00000-0xFFFFFFFF
    // ここを非キャッシュ化してCPU間共有データの事故を防ぐ
    Xil_SetTlbAttributes(core::ipc::kOcmHigh1MB, NORM_NONCACHE);
    dsb();
    isb();
}

/**
 * @brief 周辺アドレス領域の属性をDeviceに固定する
 *
 * MMU属性の誤設定で SLCR / GICなどのレジスタアクセスが壊れる事故を防ぐため、
 * 先にDeviceメモリ属性として明示する
 */
void FixPeripheralAttrsDevice()
{
    // 周辺はDeviceで固定（事故防止）
    Xil_SetTlbAttributes(0xF8000000u, DEVICE_MEMORY);  // SLCR含む
    Xil_SetTlbAttributes(0xF8F00000u, DEVICE_MEMORY);  // GIC含む
    dsb();
    isb();
}

}  // namespace

namespace core {
namespace ipc {

/**
 * @brief AMP用にOCM共有領域を初期化する
 */
void InitAmpOcmSharedSafe()
{
    // 順序が重要
    // 周辺属性固定 -> OCMウィンドウ有効化 -> 共有領域を非キャッシュ化
    // まず周辺をDevice固定
    FixPeripheralAttrsDevice();

    // OCM high windowを有効化
    EnableOcmHigh();

    // OCM highを非キャッシュに
    MakeOcmHighNonCache();
}

/**
 * @brief GIC DistributorをGroup0/Group1ともにenableする
 *
 * ICDDCR (Distributor Control Register)のGroup0/1のenableを立てる
 * - 0x01: Group0 enable
 * - 0x02: Group1 enable
 * - 0x03: Group0+Group1 enable
 */
void EnableGicDistributorGroup01()
{
    // Zynq GIC Distributor Control Register
    constexpr uint32_t ICDDCR = 0xF8F01000;
    Xil_Out32(ICDDCR, 0x03);  // Group0+1 enable
    dsb();
    isb();
}

}  // namespace ipc
}  // namespace core
