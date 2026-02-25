/**
 * @file gic_core.cpp
 * @brief gic_coreの実装
 */

// 自ヘッダー
#include "gic_core.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace platform {

/**
 * @brief GIC初期化
 *
 * @param cfg 初期化設定
 */
XStatus GicCore::Init(const Config &cfg)
{
    LOG_SCOPE();

    if (run_) {
        LOGW("Gic Already Started");
        return XST_SUCCESS;
    }

    cfg_ = cfg;

    XScuGic_Config *gic_cfg = XScuGic_LookupConfig(cfg_.base_addr);
    if (!gic_cfg) {
        LOGE("Gic LookupConfig Failed");
        return XST_DEVICE_NOT_FOUND;
    }

    XStatus st = XScuGic_CfgInitialize(&gic_, gic_cfg, gic_cfg->CpuBaseAddress);
    if (st != XST_SUCCESS) {
        LOGE("Gic CfgInitialize Failed");
        return st;
    }

    // 例外ハンドラ登録
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, (void *)&gic_);
    Xil_ExceptionEnable();

    run_ = true;

    return XST_SUCCESS;
}

/**
 * @brief GIC停止
 *
 * 例外処理を無効化して割り込みを停止。
 */
void GicCore::Deinit()
{
    if (!run_) {
        return;
    }

    // 例外を無効化
    Xil_ExceptionDisable();
    run_ = false;
}

/**
 * @brief 割り込みを接続する
 *
 * @param int_id   割り込みID
 * @param handler  割り込みハンドラ
 * @param ref      ハンドラに渡す参照
 * @param priority 優先度
 * @param trigger  トリガ種別（レベル/エッジ）
 * @return XStatus
 */
XStatus GicCore::Attach(uint32_t int_id, Xil_InterruptHandler handler, void *ref, GicPriority priority,
                        GicTrigger trigger)
{
    LOG_SCOPE();

    if (!run_) {
        LOGE("Gic is not running");
        return XST_FAILURE;
    }

    if (handler == nullptr) {
        LOGE("Gic Attach Failed: null handler");
        return XST_INVALID_PARAM;
    }

    // 宛先CPUの設定
    XScuGic_InterruptMaptoCpu(&gic_, static_cast<uint8_t>(cfg_.target_cpu), int_id);

    XScuGic_SetPriorityTriggerType(&gic_, int_id, static_cast<uint8_t>(priority), static_cast<uint8_t>(trigger));

    XStatus st = XScuGic_Connect(&gic_, int_id, handler, ref);
    if (st != XST_SUCCESS) {
        LOGE("Gic Connect Failed: int_id=%u st=%d", int_id, st);
        return st;
    }

    return XST_SUCCESS;
}

/**
 * @brief 割り込みを切断する
 *
 * @param int_id 割り込みID
 */
void GicCore::Detach(uint32_t int_id)
{
    if (!run_) {
        LOGE("Gic is not running");
        return;
    }

    XScuGic_Disconnect(&gic_, int_id);
}

/**
 * @brief 割り込みを有効化する
 *
 * @param int_id 割り込みID
 */
void GicCore::Enable(uint32_t int_id)
{
    if (!run_) {
        LOGE("Gic is not running");
        return;
    }

    XScuGic_Enable(&gic_, int_id);
}

/**
 * @brief 割り込みを無効化する
 *
 * @param int_id 割り込みID
 */
void GicCore::Disable(uint32_t int_id)
{
    if (!run_) {
        LOGE("Gic is not running");
        return;
    }

    XScuGic_Disable(&gic_, int_id);
}

}  // namespace platform
}  // namespace core0
