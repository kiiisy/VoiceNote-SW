/**
 * @file gpio_ps.cpp
 * @brief PS 側 GPIO ドライバ（MIO 制御）のラッパ実装
 *
 * Zynq-7000 の PS GPIO（XGpioPs）を、アプリ側から扱いやすい形に薄くラップする。
 *
 * - 目的: MIO ピンのレベル出力・出力設定を簡潔に行う
 * - 想定用途: LCD の DC/RST、その他ボード上の簡易 GPIO 制御
 *
 * @note
 * - 本実装は「出力専用」の最小 API を提供する（入力・割り込みなどは未実装）。
 * - XGpioPs は初期化時にベースアドレスのコンフィグ検索が必要。
 */

// 自ヘッダー
#include "gpio_ps.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core1 {
namespace platform {

/**
 * @brief PS GPIOを初期化する
 *
 * XGpioPs_LookupConfig() でベースアドレスに対応するコンフィグを取得し、
 * XGpioPs_CfgInitialize() によりドライバを初期化する。
 *
 * @return XST_SUCCESS 成功
 * @return XST_FAILURE 失敗（コンフィグが見つからない等）
 */
int32_t GpioPs::Init()
{
    LOG_SCOPE();

    XGpioPs_Config *cfg = XGpioPs_LookupConfig(base_addr_);
    if (!cfg) {
        LOGE("GPIO cfg not found");
        return XST_FAILURE;
    }

    return XGpioPs_CfgInitialize(&gpio_, cfg, cfg->BaseAddr);
}

/**
 * @brief 指定 MIO ピンに出力レベルを書き込む
 *
 * @param mio   対象のMIOピン番号
 * @param level 出力レベル（0=Low, 非0=High）
 */
void GpioPs::WritePin(uint32_t mio, uint32_t level) { XGpioPs_WritePin(&gpio_, mio, level ? 1 : 0); }

/**
 * @brief MIOピンを出力に設定する
 *
 * 方向を出力にし、出力許可（OutputEnable）も有効化する。
 *
 * @param mio 対象のMIOピン番号
 */
void GpioPs::SetOutput(uint32_t mio)
{
    XGpioPs_SetDirectionPin(&gpio_, mio, 1);
    XGpioPs_SetOutputEnablePin(&gpio_, mio, 1);
}
}  // namespace platform
}  // namespace core1
