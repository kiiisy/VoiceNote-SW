/**
 * @file record_controller.h
 * @brief record_controllerの定義
 */

#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "audio_bpp_pool.h"
#include "audio_formatter_rx.h"
#include "ipc_msg.h"
#include "sd_bpp_recorder.h"
#include "wav_writer.h"

namespace core0 {
namespace app {

/**
 * @brief Record制御
 */
class RecordController final
{
public:
    enum class State : uint8_t
    {
        kIdle = 0,
        kRunning,
        kStopping,
        kError,
    };

    enum class Event : uint8_t
    {
        kNone = 0,
        kStarted,
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
        kRecorderInitFailed = -2,
        kRxDrop             = -10,
    };

    void Bind(module::AudioFormatterRx &rx, module::AudioBppPool &rx_pool);
    void Reset();
    bool Start(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch);
    void Process();
    bool IsActive() const { return state_ != State::kIdle; }
    bool PopEvent(EventInfo *out);
    bool GetStatus(core::ipc::RecordStatusPayload *out) const;
    void RequestStop() { stop_req_ = true; };

private:
    void     SetNextState(State state) { state_ = state; };
    void     Notify(Event type, int32_t err);
    void     ExecuteRunning();
    void     ExecuteStopping();
    uint32_t CopySideBufferToPool(module::AudioFormatterRx::BufferSide which);

    // 停止要求が来なければこの秒数で自動停止する
    static constexpr uint16_t kMaxRecordSeconds = 30;

    module::AudioFormatterRx *rx_{nullptr};
    module::AudioBppPool     *rx_pool_{nullptr};
    module::SdBppRecorder     recorder_;

    State     state_{State::kIdle};  // 状態管理
    EventInfo event_{};              // イベント管理
    bool      event_pending_{false};

    bool stop_req_{false};  // 停止要求フラグ

    uintptr_t rx_ring_base_{0};

    uint32_t written_bytes_{0};  // 書き込みしたByte数
    uint32_t target_bytes_{0};   // 目標Byte数

    uint32_t rx_drop_count_{0};
    bool     drop_pending_{false};  // 直近tickでドロップが起きた

    uint32_t bytes_per_sec_{0};
};

}  // namespace app
}  // namespace core0
