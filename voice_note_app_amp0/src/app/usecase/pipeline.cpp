/**
 * @file pipeline.cpp
 * @brief Pipelineの実装
 */

// 自ヘッダー
#include "pipeline.h"

// プロジェクトライブラリ
#include "acu_core.h"
#include "agc_core.h"
#include "audio_formatter_core.h"
#include "audio_formatter_rx.h"
#include "audio_formatter_tx.h"
#include "codec_provider.h"
#include "common.h"
#include "i2s_rx_core.h"
#include "i2s_tx_core.h"
#include "logger_core.h"

namespace core0 {
namespace app {

/**
 * @brief 初期化処理
 *
 * @retval true  初期化成功
 * @retval false 初期化失敗
 */
bool Pipeline::Init(uintptr_t base_addr, platform::I2sTx &i2s_tx, platform::I2sRx &i2s_rx)
{
    LOG_SCOPE();

    auto &af_core = module::AudioFormatterCore::GetInstance();
    if (!af_core.Init({kAudioFormatterBaseAddr})) {
        LOGE("Failed to initialize AudioFormatterCore");
        return false;
    }

    const module::AudioFormatterTx::Config tx_cfg = {
        kAudioFormatterTxIrqId, base_addr, kChannelCount, BIT_DEPTH_16, kPeriodsPerCh, kPeriodsBytes,
    };

    tx_ = &module::AudioFormatterTx::GetInstance();
    if (!tx_->Init(tx_cfg)) {
        LOGE("Failed to initialize AudioFormatterTx");
        return false;
    }

    const module::AudioFormatterRx::Config rx_cfg = {
        kAudioFormatterRxIrqId, base_addr, kChannelCount, BIT_DEPTH_16, kPeriodsPerCh, kPeriodsBytes,
    };

    rx_ = &module::AudioFormatterRx::GetInstance();
    if (!rx_->Init(rx_cfg)) {
        LOGE("Failed to initialize AudioFormatterRx");
        return false;
    }

    auto &codec = module::CodecProvider::GetInstance().Get();
    if (!codec.Init()) {
        LOGE("Codec initialization failed");
        return false;
    }

    if (i2s_rx.Init({kI2sRxBaseAddr, I2sMuxBaseAddr, kI2sRxIrqId}) != XST_SUCCESS) {
        LOGE("I2s RX Initialization Failed");
        return false;
    }

    if (i2s_tx.Init({kI2sTxBaseAddr, I2sMuxBaseAddr, kI2sTxIrqId}) != XST_SUCCESS) {
        LOGE("I2s TX Initialization Failed");
        return false;
    }

    auto &agc = platform::Agc::GetInstance();
    agc.Init(kAgcBaseAddr);
    agc.ApplyDefault();

    auto &acu = platform::Acu::GetInstance();
    acu.Init(kAcuBaseAddr);
    acu.ApplyDefault();

    return true;
}

/**
 * @brief 終了処理
 */
void Pipeline::Deinit()
{
    if (rx_) {
        rx_->Deinit();
    }
    if (tx_) {
        tx_->Deinit();
    }

    rx_ = nullptr;
    tx_ = nullptr;
}

}  // namespace app
}  // namespace core0
