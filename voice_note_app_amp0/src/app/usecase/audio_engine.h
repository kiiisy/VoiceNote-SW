/**
 * @file audio_engine.h
 * @brief AudioEngineの定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_bpp_pool.h"
#include "audio_notification.h"
#include "common.h"
#include "fsm.h"
#include "pipeline.h"
#include "playback_controller.h"
#include "record_controller.h"

namespace core0 {
namespace platform {
class I2sTx;
class I2sRx;
}  // namespace platform

namespace app {

class AudioEngine final
{
public:
    explicit AudioEngine(platform::I2sTx &i2s_tx, platform::I2sRx &i2s_rx) : i2s_tx_(i2s_tx), i2s_rx_(i2s_rx) {}
    ~AudioEngine() { Deinit(); }

    bool Init();
    void Deinit();
    bool RequestPlay(const char *wav_path);
    bool RequestRecord(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch);
    bool RequestRecordStop();
    void RequestPause();
    void RequestResume();

    void Task();
    bool GetNextNotification(AudioNotification *noti) { return notification_queue_.Dequeue(noti); }

    bool GetStatus(core::ipc::PlaybackStatusPayload *out) { return pb_ctrl_.GetStatus(out); };
    bool GetStatus(core::ipc::RecordStatusPayload *out) { return rec_ctrl_.GetStatus(out); };

    bool IsPlaybackActive() const;
    bool IsRecordActive() const;

private:
    void RunFsm();
    void ExecuteFsmAction(AudioAction action);
    void ProcessRecord();
    void ProcessPlayback();

    // エラー系の用途未だ未定
    enum class Error : int32_t
    {
        kNone                = 0,
        kPlaybackStartFailed = -100,
        kRecordStartFailed   = -200,
    };

    uintptr_t ddr_base_addr_ = kDmaBufBasePhys;

    // 外部モジュール
    module::AudioBppPool tx_pool_;
    module::AudioBppPool rx_pool_;
    platform::I2sTx     &i2s_tx_;
    platform::I2sRx     &i2s_rx_;

    // パス管理
    static constexpr size_t kPathMax             = 128;
    char                    play_path_[kPathMax] = {};
    char                    rec_path_[kPathMax]  = {};

    // 内部モジュール
    Pipeline               pipeline_;
    AudioNotificationQueue notification_queue_;
    PlaybackController     pb_ctrl_;
    RecordController       rec_ctrl_;

    // FSM系の定義
    struct FsmContext
    {
        struct ActionParams
        {
            const char *play_path      = nullptr;
            const char *rec_path       = nullptr;
            uint32_t    sample_rate_hz = kSampleRate;
            uint16_t    bits           = kBitDepth;
            uint16_t    ch             = kChannelCount;
        };

        AudioFsm      fsm{};
        ActionParams  params{};
        bool          has_event{false};
        AudioFsmEvent event{AudioFsmEvent::UiPlayPressed};

        void Reset()
        {
            fsm       = AudioFsm{};
            params    = {};
            has_event = false;
            event     = AudioFsmEvent::UiPlayPressed;
        }

        void SetEvent(AudioFsmEvent ev)
        {
            has_event = true;
            event     = ev;
        }

        bool Dispatch(AudioTransition *transition)
        {
            if (!has_event || !transition) {
                return false;
            }

            has_event   = false;
            *transition = fsm.Dispatch(event);
            return true;
        }
    };

    FsmContext fsm_ctx_{};
};

}  // namespace app
}  // namespace core0
