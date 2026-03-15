/**
 * @file audio_ui_input_handler.cpp
 * @brief AudioUiInputHandlerの実装
 */

// 自ヘッダー
#include "audio_ui_input_handler.h"

// 標準ライブラリ
#include <cstdio>
#include <cstring>

// プロジェクトライブラリ
#include "system.h"
#include "fsm.h"
#include "logger_core.h"
#include "utility.h"

namespace core0 {
namespace app {

/**
 * @brief 再生要求を受け付けてSystemへ委譲する
 *
 * @param[in] wav_path 再生対象WAVパス
 * @retval true  要求受理
 * @retval false 引数不正またはデバウンス抑止
 */
bool AudioUiInputHandler::RequestPlay(const char *wav_path)
{
    if (!wav_path) {
        return false;
    }

    const uint32_t now = GetTimeMs();
    if ((now - last_ui_play_req_ms_) < kUiPlayDebounceMs) {
        LOGW("Ignore duplicated UiPlayPressed within debounce window");
        return false;
    }
    last_ui_play_req_ms_ = now;

    std::strncpy(play_path_, wav_path, kPathMax - 1U);
    play_path_[kPathMax - 1U] = '\0';

    return sys_.EnqueuePlay(play_path_);
}

/**
 * @brief 録音要求を受け付けてSystemへ委譲する
 *
 * @param[in] wav_path       録音出力WAVパス
 * @param[in] sample_rate_hz サンプルレート
 * @param[in] bits           ビット深度
 * @param[in] ch             チャネル数
 * @retval true  要求受理
 * @retval false 引数不正またはデバウンス抑止
 */
bool AudioUiInputHandler::RequestRecord(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch)
{
    if (!wav_path) {
        return false;
    }

    const uint32_t now = GetTimeMs();
    if ((now - last_ui_rec_req_ms_) < kUiRecDebounceMs) {
        LOGW("Ignore duplicated UiRecPressed within debounce window");
        return false;
    }
    last_ui_rec_req_ms_ = now;

    std::strncpy(rec_path_, wav_path, kPathMax - 1U);
    rec_path_[kPathMax - 1U] = '\0';

    return sys_.EnqueueRecord(rec_path_, sample_rate_hz, bits, ch);
}

/**
 * @brief 自動採番したファイル名で録音要求を発行する
 *
 * @param[in] sample_rate_hz サンプルレート
 * @param[in] bits           ビット深度
 * @param[in] ch             チャネル数
 * @retval true  要求受理
 * @retval false 要求失敗
 */
bool AudioUiInputHandler::RequestRecordAuto(uint32_t sample_rate_hz, uint16_t bits, uint16_t ch)
{
    std::snprintf(rec_path_, sizeof(rec_path_), "/record_%05u.wav", (unsigned)next_rec_index_);

    if (!RequestRecord(rec_path_, sample_rate_hz, bits, ch)) {
        return false;
    }

    if (next_rec_index_ < kMaxRecIndex) {
        ++next_rec_index_;
    } else {
        next_rec_index_ = 1;
    }

    return true;
}

/**
 * @brief 録音停止要求を発行する
 *
 * @retval true  要求受理
 * @retval false 状態不一致で要求不可
 */
bool AudioUiInputHandler::RequestRecordStop()
{
    if (sys_.GetState() != AudioState::Recording) {
        return false;
    }
    sys_.EnqueueRecToggle();
    return true;
}

/**
 * @brief 再生一時停止要求を発行する
 *
 * @retval true  要求受理
 * @retval false 状態不一致で要求不可
 */
bool AudioUiInputHandler::RequestPause()
{
    if (sys_.GetState() != AudioState::Playing) {
        return false;
    }
    sys_.EnqueuePlayToggle();
    return true;
}

/**
 * @brief 再生再開要求を発行する
 *
 * @retval true  要求受理
 * @retval false 状態不一致で要求不可
 */
bool AudioUiInputHandler::RequestResume()
{
    if (sys_.GetState() != AudioState::Paused) {
        return false;
    }
    sys_.EnqueuePlayToggle();
    return true;
}

}  // namespace app
}  // namespace core0
