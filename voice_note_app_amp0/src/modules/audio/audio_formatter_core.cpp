/**
 * @file audio_formatter_core.cpp
 * @brief AudioFormatterCoreの実装
 */

// 自ヘッダー
#include "audio_formatter_core.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace module {

/**
 * @brief Audio Formatter IPを初期化する（1回だけ呼ぶ想定）
 *
 * @param[in] cfg 初期化設定
 * @retval true  成功（または既に初期化済み）
 * @retval false 失敗
 */
bool AudioFormatterCore::Init(const Config &cfg)
{
    if (initialized_) {
        if (base_addr_ != cfg.base_addr) {
            LOGE("AudioFormatterCore already initialized with different base addr");
            return false;
        }
        // 再初期化不要
        return true;
    }

    auto *xaudio_cfg = XAudioFormatter_LookupConfig(cfg.base_addr);
    if (!xaudio_cfg) {
        LOGE("Audio Formatter S2MM LookupConfig Failed");
        return false;
    }

    XStatus status = XAudioFormatter_CfgInitialize(&core_, xaudio_cfg);
    if (status != XST_SUCCESS) {
        LOGE("AudioFormatterCore: CfgInitialize failed");
        return false;
    }

    base_addr_   = cfg.base_addr;
    initialized_ = true;

    return true;
}

}  // namespace module
}  // namespace core0
