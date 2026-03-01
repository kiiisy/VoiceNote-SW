/**
 * @file audio_engine.h
 * @brief AudioEngineの定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_bpp_pool.h"
#include "audio_formatter_rx.h"
#include "audio_formatter_tx.h"
#include "audio_notification.h"
#include "codec_provider.h"
#include "common.h"
#include "fsm.h"
#include "i2s_rx_core.h"
#include "i2s_tx_core.h"
#include "notification_queue.h"
#include "pipeline.h"
#include "playback_controller.h"
#include "record_controller.h"

namespace core0 {
namespace app {

class AudioEngine final
{
public:
    explicit AudioEngine(module::AudioCodec &codec, platform::I2sTx &i2s_tx, platform::I2sRx &i2s_rx)
        : codec_(codec), i2s_tx_(i2s_tx), i2s_rx_(i2s_rx)
    {
    }
    ~AudioEngine() { Deinit(); }

    bool Init();
    void Deinit();
    bool RequestPlay(const char *wav_path);
    bool RequestRecord(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch);
    bool RequestRecordStop();
    void RequestPause();
    void RequestResume();

    void Task();
    bool GetNextNotification(AudioNotification *noti);

    bool GetStatus(core::ipc::PlaybackStatusPayload *out) { return pb_ctrl_.GetStatus(out); };
    bool GetStatus(core::ipc::RecordStatusPayload *out) { return rec_ctrl_.GetStatus(out); };

    bool IsPlaybackActive() const;
    bool IsRecordActive() const;

private:
    void RunFsm();
    void ExecuteAction(AudioAction action);
    void ProcessRecord();
    void ProcessPlayback();
    void PushFsmEvent(AudioFsmEvent event);
    void PublishNotification(AudioNotification::Type type, int32_t err);

    enum class Error : int32_t
    {
        kNone                = 0,
        kPlaybackStartFailed = -100,
        kRecordStartFailed   = -200,
    };

    uintptr_t ddr_base_addr_ = kDmaBufBasePhys;

    // 外部モジュール
    module::AudioBppPool      tx_pool_;
    module::AudioBppPool      rx_pool_;
    module::AudioFormatterTx *tx_{nullptr};
    module::AudioFormatterRx *rx_{nullptr};
    module::AudioCodec       &codec_;
    platform::I2sTx          &i2s_tx_;
    platform::I2sRx          &i2s_rx_;

    // パス管理
    static constexpr size_t kPathMax             = 128;
    char                    play_path_[kPathMax] = {};
    char                    rec_path_[kPathMax]  = {};

    // 内部モジュール
    Pipeline               pipeline_;
    AudioNotificationQueue notification_queue_;
    PlaybackController     pb_ctrl_;
    RecordController       rec_ctrl_;

    struct Params
    {
        const char *play_path      = nullptr;
        const char *rec_path       = nullptr;
        uint32_t    sample_rate_hz = 48000;
        uint16_t    bits           = 16;
        uint16_t    ch             = 2;
    };

    AudioFsm      fsm_{};
    Params        fsm_params_{};
    bool          event_pending_{false};
    AudioFsmEvent event_{AudioFsmEvent::UiPlayPressed};
};

}  // namespace app
}  // namespace core0
