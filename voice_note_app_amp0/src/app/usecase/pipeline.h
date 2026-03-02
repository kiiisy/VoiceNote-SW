/**
 * @file pipeline.h
 * @brief Pipelineの定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {

namespace module {
class AudioFormatterTx;
class AudioFormatterRx;
}  // namespace module

namespace platform {
class Acu;
class I2sTx;
class I2sRx;
}  // namespace platform

namespace app {

/**
 * @brief AudioHWパイプライン初期化の束ね役クラス
 */
class Pipeline final
{
public:
    bool Init(uintptr_t base_addr, platform::Acu &acu, uintptr_t acu_baseaddr, platform::I2sTx &i2s_tx,
              platform::I2sRx &i2s_rx);

    void Deinit();

    module::AudioFormatterTx *tx() const { return tx_; }
    module::AudioFormatterRx *rx() const { return rx_; }
    platform::Acu            *acu() const { return acu_; }

private:
    module::AudioFormatterTx *tx_{nullptr};
    module::AudioFormatterRx *rx_{nullptr};
    platform::Acu            *acu_{nullptr};
};

}  // namespace app
}  // namespace core0
