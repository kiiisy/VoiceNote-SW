/**
 * @file spi_ps.cpp
 * @brief spi_psの実装
 */

// 自ヘッダー
#include "spi_ps.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core1 {
namespace platform {

/**
 * @brief PS SPIを初期化する
 *
 * @return XST_SUCCESS成功、それ以外はXilinxドライバのエラーコード
 */
int32_t SpiPs::Init()
{
    LOG_SCOPE();

    auto *xspi_cfg = XSpiPs_LookupConfig(cfg_.base_addr);
    if (!xspi_cfg) {
        LOGE("SpiPs LookupConfig Failed.");
        return XST_FAILURE;
    }

    int32_t status = XSpiPs_CfgInitialize(&spi_, xspi_cfg, xspi_cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        LOGE("SpiPs CfgInitialize Failed.");
        return status;
    }

    // 起動直後のドライバ状態チェック（問題時は早めに検知）
    status = XSpiPs_SelfTest(&spi_);
    if (status != XST_SUCCESS) {
        LOGE("SpiPs SelfTest Failed.");
        return status;
    }

    status = XSpiPs_SetOptions(&spi_, cfg_.options);
    if (status != XST_SUCCESS) {
        LOGE("SpiPs SetOptions Failed.");
        return status;
    }

    status = XSpiPs_SetSlaveSelect(&spi_, cfg_.slave_select);
    if (status != XST_SUCCESS) {
        LOGE("SpiPs SetSlaveSelect Failed.");
        return status;
    }

    status = XSpiPs_SetClkPrescaler(&spi_, cfg_.clk_prescaler);
    if (status != XST_SUCCESS) {
        LOGE("SpiPs SetClkPrescaler Failed.");
        return status;
    }

    return XST_SUCCESS;
}

/**
 * @brief SPIポーリング転送を行う
 *
 * @param tx 送信バッファ
 * @param rx 受信バッファ
 * @param len 転送バイト数
 * @return XST_SUCCESS 成功、それ以外は Xilinx ドライバのエラーコード
 */
int32_t SpiPs::Transfer(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    if (len == 0U) {
        return XST_SUCCESS;
    }

    if ((tx == nullptr) && (rx == nullptr)) {
        LOGE("SpiPs Transfer invalid buffer");
        return XST_FAILURE;
    }

    return XSpiPs_PolledTransfer(&spi_, const_cast<uint8_t *>(tx), rx, len);
}

}  // namespace platform
}  // namespace core1
