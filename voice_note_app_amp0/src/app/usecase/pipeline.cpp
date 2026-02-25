// 自ヘッダー
#include "pipeline.h"

// プロジェクトライブラリ
#include "acu_core.h"
#include "audio_formatter_core.h"
#include "audio_formatter_rx.h"
#include "audio_formatter_tx.h"
#include "codec_provider.h"
#include "common.h"
#include "logger_core.h"

namespace core0 {
namespace app {

bool Pipeline::Init(uintptr_t base_addr, module::AudioCodec &codec, platform::Acu &acu, uintptr_t acu_baseaddr)
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

    if (!codec.Init()) {
        LOGE("Codec initialization failed");
        return false;
    }

    acu_ = &acu;
    acu_->Init(acu_baseaddr);
    acu_->ApplyDefault();

    return true;
}

void Pipeline::Deinit()
{
    if (rx_) {
        rx_->Deinit();
    }
    if (tx_) {
        tx_->Deinit();
    }

    rx_  = nullptr;
    tx_  = nullptr;
    acu_ = nullptr;
}

}  // namespace app
}  // namespace core0
