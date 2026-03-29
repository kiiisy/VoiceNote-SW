/**
 * @file system.h
 * @brief Systemの定義
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "app_server.h"
#include "audio_bpp_pool.h"
#include "audio_dsp_config_handler.h"
#include "audio_event_bus.h"
#include "audio_file_query_handler.h"
#include "audio_format.h"
#include "audio_ipc_command_handler.h"
#include "audio_ipc_status_handler.h"
#include "audio_notification.h"
#include "audio_notification_publisher.h"
#include "audio_ui_input_handler.h"
#include "ddr_audio_buffer.h"
#include "fsm.h"
#include "memory_map.h"
#include "pipeline.h"
#include "playback_controller.h"
#include "record_controller.h"

namespace core0 {
namespace platform {
class I2sTx;
class I2sRx;
}  // namespace platform

namespace app {

class System final
{
public:
    System(core0::ipc::AppServer &server, platform::I2sTx &i2s_tx, platform::I2sRx &i2s_rx)
        : server_(server), i2s_tx_(i2s_tx), i2s_rx_(i2s_rx), ui_input_handler_(*this),
          command_handler_(server_, ui_input_handler_), dsp_config_handler_(server_),
          file_query_handler_(server_, *this), status_handler_(server_, *this)
    {
    }
    ~System() { Deinit(); }

    bool Init();
    void Deinit();
    bool EnqueuePlay(const char *wav_path);
    bool EnqueueRecord(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch);
    void EnqueuePlayToggle();
    void EnqueueRecToggle();

    void SetHandlers();
    void Run();
    void Task();
    bool GetNextNotification(AudioNotification *noti) { return notification_publisher_.Pop(noti); }

    bool GetStatus(core::ipc::PlaybackStatusPayload *out) { return pb_ctrl_.GetStatus(out); };
    bool GetStatus(core::ipc::RecordStatusPayload *out) { return rec_ctrl_.GetStatus(out); };

    bool       IsPlaybackActive() const;
    bool       IsRecordActive() const;
    AudioState GetState() const { return fsm_ctx_.fsm.GetState(); }

private:
    void RunFsm();
    void ExecuteFsmAction(AudioAction action);
    void ProcessRecord();
    void ProcessPlayback();
    bool StartRecordNow();
    bool StartAutoRecordWait();
    void StopAutoRecordWait();
    void ProcessAutoRecordWait();

    // エラー系の用途未だ未定
    enum class Error : int32_t
    {
        kNone                = 0,
        kPlaybackStartFailed = -100,
        kRecordStartFailed   = -200,
    };

    // お試し：内部固定フラグで経路を切替（IPC切替へ移行）
    static constexpr bool kUseDdrPlayback = true;
    static constexpr bool kUseDdrRecord   = true;

    uintptr_t ddr_base_addr_ = kDmaBufBasePhys;

    // 外部モジュール
    module::DdrAudioBuffer ddr_audio_buf_;
    module::AudioBppPool   tx_pool_;
    module::AudioBppPool   rx_pool_;
    core0::ipc::AppServer &server_;
    platform::I2sTx       &i2s_tx_;
    platform::I2sRx       &i2s_rx_;

    // 内部モジュール
    Pipeline                   pipeline_;
    AudioNotificationPublisher notification_publisher_;
    PlaybackController         pb_ctrl_;
    RecordController           rec_ctrl_;
    AudioUiInputHandler        ui_input_handler_;
    AudioIpcCommandHandler     command_handler_;
    AudioDspConfigHandler      dsp_config_handler_;
    AudioFileQueryHandler      file_query_handler_;
    AudioIpcStatusHandler      status_handler_;

    // 自動録音
    bool arec_waiting_{false};
    bool arec_started_{false};

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
        AudioEventBus event_bus{};

        void Reset()
        {
            fsm    = AudioFsm{};
            params = {};
            event_bus.Reset();
        }

        void SetEvent(AudioFsmEvent ev) { event_bus.SetEvent(ev); }

        bool Dispatch(AudioTransition *transition)
        {
            if (!transition) {
                return false;
            }

            AudioFsmEvent event{};
            if (!event_bus.Pop(&event)) {
                return false;
            }

            *transition = fsm.Dispatch(event);
            return true;
        }
    };

    FsmContext fsm_ctx_{};
};

}  // namespace app
}  // namespace core0
