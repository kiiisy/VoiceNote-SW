/**
 * @file audio_ui_input_handler.h
 * @brief 外部入力受付を扱うハンドラ
 */
#pragma once

// 標準ライブラリ
#include <cstddef>
#include <cstdint>

namespace core0 {
namespace app {

class System;

/**
 * @brief IPC/UI要求を受けてSystemへ委譲する
 */
class AudioUiInputHandler final
{
public:
    explicit AudioUiInputHandler(System &sys) : sys_(sys) {}

    bool RequestPlay(const char *wav_path);
    bool RequestRecord(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch);
    bool RequestRecordAuto(uint32_t sample_rate_hz, uint16_t bits, uint16_t ch);
    bool RequestRecordStop();
    bool RequestPause();
    bool RequestResume();

private:
    static constexpr uint32_t kUiPlayDebounceMs = 180;
    static constexpr uint32_t kUiRecDebounceMs  = 180;
    static constexpr uint32_t kMaxRecIndex      = 99999;
    static constexpr size_t   kPathMax          = 128;

    uint32_t last_ui_play_req_ms_{0};
    uint32_t last_ui_rec_req_ms_{0};
    uint32_t next_rec_index_{1};
    char     play_path_[kPathMax]{};
    char     rec_path_[kPathMax]{};

    System &sys_;
};

}  // namespace app
}  // namespace core0
