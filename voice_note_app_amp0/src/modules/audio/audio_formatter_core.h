/**
 * @file audio_formatter_core.h
 * @brief Audio Formatter IP（XAudioFormatter）ラッパークラス
 */
#pragma once

#ifdef __cplusplus
#define _Bool bool
#endif

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xaudioformatter.h"

namespace core0 {
namespace module {

/**
 * @class AudioFormatterCore
 * @brief Audio Formatter IP制御クラス
 */
class AudioFormatterCore final
{
public:
    struct Config
    {
        uint32_t base_addr{};
    };

    static AudioFormatterCore &GetInstance()
    {
        static AudioFormatterCore instance;
        return instance;
    }

    bool Init(const Config &cfg);

    XAudioFormatter *GetCore() { return initialized_ ? &core_ : nullptr; }
    uintptr_t        GetBaseAddr() const { return base_addr_; }

private:
    AudioFormatterCore()                                      = default;
    AudioFormatterCore(const AudioFormatterCore &)            = delete;
    AudioFormatterCore &operator=(const AudioFormatterCore &) = delete;

    XAudioFormatter core_{};              // XAudioFormatter ドライバハンドル
    bool            initialized_{false};  // 初期化完了フラグ（Init 成功後 true）
    uintptr_t       base_addr_{0};        // IP のベースアドレス（参照用）
};

}  // namespace module
}  // namespace core0
