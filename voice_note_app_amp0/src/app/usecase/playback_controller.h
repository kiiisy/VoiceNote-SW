/**
 * @file playback_controller.h
 * @brief playback_controllerの定義
 */

#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_bpp_pool.h"
#include "audio_formatter_tx.h"
#include "common.h"
#include "ipc_msg.h"
#include "sd_bpp_feeder.h"
#include "wav_reader.h"

namespace core0 {
namespace app {

/**
 * @brief Playback制御
 */
class PlaybackController final
{
public:
    enum class State : uint8_t
    {
        kIdle = 0,
        kRunning,
        kPaused,
        kFinishing,
        kError,
    };

    enum class Event : uint8_t
    {
        kNone = 0,
        kStarted,
        kPaused,
        kResumed,
        kStopped,
        kError,
    };

    struct EventInfo
    {
        Event   type = Event::kNone;
        int32_t err  = 0;
    };

    enum class Error : int32_t
    {
        kNone               = 0,
        kInvalidParam       = -1,
        kFeederInitFailed   = -2,
        kInvalidBytesPerSec = -3,
    };

    void Bind(module::AudioFormatterTx &tx, module::AudioBppPool &tx_pool);
    void Reset();
    bool Start(const char *wav_path);
    void Process();
    bool IsActive() const { return state_ != State::kIdle; }
    bool GetNextEvent(EventInfo *event);
    bool GetStatus(core::ipc::PlaybackStatusPayload *status) const;
    void RequestPause() { pause_req_ = true; };
    void RequestResume() { resume_req_ = true; };

private:
    void SetNextState(State state) { state_ = state; };
    void Notify(Event type, int32_t err);
    void ExecuteRunning();
    void ExecutePaused();
    void ExecuteFinishing();
    void FillSide(module::AudioFormatterTx::BufferSide which);
    void Finish(Event event, int32_t err = 0);

private:
    module::AudioFormatterTx *tx_{nullptr};
    module::AudioBppPool     *tx_pool_{nullptr};
    module::SdBppFeeder       feeder_;

    State     state_{State::kIdle};  // 状態管理
    EventInfo event_{};              // イベント管理
    bool      event_pending_{false};

    bool pause_req_{false};   // 一時停止フラグ
    bool resume_req_{false};  // 再生再開フラグ

    uintptr_t tx_ring_base_{0};

    uint32_t total_bytes_{0};          // 総Byte数
    uint32_t submitted_bytes_{0};      // 投入済みByte数
    bool     finishing_{false};        // 終了中か否か
    bool     needs_final_ioc_{false};  // 最後のIOCを待つか否か
    uint32_t fin_tick_{0};             // 終了処理の経過tick

    uint32_t bytes_per_sec_{0};  // Byte/秒
};

}  // namespace app
}  // namespace core0
