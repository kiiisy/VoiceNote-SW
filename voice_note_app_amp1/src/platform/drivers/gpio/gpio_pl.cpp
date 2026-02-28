/**
 * @file gpio_pl.cpp
 * @brief AXI GPIOドライバ実装
 */

// 自ヘッダー
#include "gpio_pl.h"

// プロジェクトライブラリ
#include "gic_core.h"
#include "logger_core.h"

namespace core1 {
namespace platform {

/**
 * @brief AXI GPIOを初期化する
 *
 * - XGpioの初期化
 * - 入力チャネル/出力チャネルの方向設定
 *
 * @retval true  初期化成功
 * @retval false 初期化失敗
 */
bool GpioPl::Init()
{
    LOG_SCOPE();

    auto *cfg = XGpio_LookupConfig(cfg_.addr);
    if (!cfg) {
        LOGE("PL Gpio LookupConfig Failed");
        return false;
    }

    if (XGpio_CfgInitialize(&inst_, cfg, cfg->BaseAddress) != XST_SUCCESS) {
        LOGE("PL Gpio CfgInitialize Failed");
        return false;
    }

    // 方向設定
    XGpio_SetDataDirection(&inst_, cfg_.ch_0, in_setting_);
    XGpio_SetDataDirection(&inst_, cfg_.ch_2, out_setting_);

    return true;
}

/**
 * @brief GPIOを停止し、割り込みを解除する
 */
void GpioPl::Deinit()
{
    if (irq_attached_) {
        auto &gic = GicCore::GetInstance();
        gic.Disable(cfg_.irq_id);
        gic.Detach(cfg_.irq_id);
        irq_attached_ = false;
    }
}

/**
 * @brief ピン方向設定
 *
 * AXI GPIOはチャネル単位で方向を持つため、
 * ピン単位の方向制御はサポートしない。
 *
 * @param pin GPIOピン番号
 * @param d   方向
 */
void GpioPl::SetDir(uint32_t pin, GpioDir d)
{
    (void)pin;
    (void)d;
}

/**
 * @brief 入力ピンの状態を読む
 *
 * @param pin ピン番号
 * @return 0 or 1
 */
uint32_t GpioPl::ReadPin(uint32_t pin)
{
    uint32_t v = XGpio_DiscreteRead(&inst_, cfg_.ch_0);
    return (v >> pin) & 1u;
}

/**
 * @brief 出力ピンに値を書き込む
 *
 * @param pin ピン番号
 * @param val 出力値（0 or 1）
 */
void GpioPl::WritePin(uint32_t pin, uint32_t val)
{
    uint32_t cur = XGpio_DiscreteRead(&inst_, cfg_.ch_2);

    if (val) {
        cur |= (1u << pin);
    } else {
        cur &= ~(1u << pin);
    }

    XGpio_DiscreteWrite(&inst_, cfg_.ch_2, cur);
}

/**
 * @brief 入力チャネル全体を読む
 *
 * @return 入力ビットマスク
 */
uint32_t GpioPl::ReadInputs() { return XGpio_DiscreteRead(&inst_, cfg_.ch_0); }

/**
 * @brief 出力チャネル全体を書き換える
 *
 * @param mask 出力ビットマスク
 */
void GpioPl::WriteOutputs(uint32_t mask) { XGpio_DiscreteWrite(&inst_, cfg_.ch_2, mask); }

/**
 * @brief GPIO割り込みを有効化し、GICに接続する
 *
 * @retval true  成功
 * @retval false 失敗
 */
bool GpioPl::EnableIrq()
{
    // 割り込みのみ有効化
    if (!irq_attached_) {
        auto &gic = GicCore::GetInstance();
        if (gic.Attach(cfg_.irq_id, (Xil_InterruptHandler)&GpioPl::IsrThunk, this, GicCore::GicPriority::Normal,
                       GicCore::GicTrigger::LevelHigh) == XST_SUCCESS) {

            gic.Enable(cfg_.irq_id);
            irq_attached_ = true;
        } else {
            LOGE("Attaching an interrupt signal failed");
            return false;
        }
    }

    XGpio_InterruptEnable(&inst_, cfg_.ch_0);
    XGpio_InterruptGlobalEnable(&inst_);

    return true;
}

/**
 * @brief GPIO割り込みを再有効化する
 */
void GpioPl::ReenableIrq()
{
    XGpio_InterruptEnable(&inst_, cfg_.ch_0);
    XGpio_InterruptGlobalEnable(&inst_);
}

/**
 * @brief GPIO割り込みを無効化する
 */
void GpioPl::DisableIrq()
{
    XGpio_InterruptDisable(&inst_, cfg_.ch_0);
    XGpio_InterruptGlobalDisable(&inst_);
}

/**
 * @brief 割り込みコールバック
 *
 * @param ref this ポインタ
 */
void GpioPl::IsrThunk(void *ref) { static_cast<GpioPl *>(ref)->OnIsr(); }

/**
 * @brief GPIO割り込み実処理
 *
 * - 割り込みを一旦無効化
 * - ステータスクリア
 * - 登録済みコールバックを実行
 */
void GpioPl::OnIsr()
{
    // 割り込みを無効化する
    DisableIrq();

    // 発生要因クリア（チャネル単位）
    uint32_t st = XGpio_InterruptGetStatus(&inst_);

    XGpio_InterruptClear(&inst_, cfg_.ch_0);

    // コールバック実行
    if (cb_) {
        cb_();
    }
}
}  // namespace platform
}  // namespace core1
