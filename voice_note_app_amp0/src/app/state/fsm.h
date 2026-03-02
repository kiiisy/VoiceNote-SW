#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {
namespace app {

/**
 * @brief AudioEngineの外部に見せる状態
 */
enum class AudioState : uint8_t
{
    Idle = 0,   // 再生も録音もしていない
    Playing,    // 再生中
    Paused,     // 再生一時停止中
    Recording,  // 録音中

    Max
};

/**
 * @brief UI/内部完了を正規化したイベント
 */
enum class AudioFsmEvent : uint8_t
{
    UiPlayPressed = 0,  // 再生トグル
    UiRecPressed,       // 録音トグル
    PbEnded,            // 再生終了
    RecEnded,           // 録音終了

    Max
};

/**
 * @brief FSMが外に依頼するアクション
 */
enum class AudioAction : uint16_t
{
    None = 0,

    // PB control
    StartPlayback,
    PausePlayback,
    ResumePlayback,

    // REC control
    StartRecord,
    StopRecord,

    // notifications
    EmitPlaybackStarted,
    EmitPlaybackPaused,
    EmitPlaybackResumed,
    EmitPlaybackStopped,
    EmitRecordStarted,
    EmitRecordStopped,

    Max
};

struct AudioTransition
{
    AudioState  next;
    AudioAction action1;
    AudioAction action2;
};

class AudioFsm final
{
public:
    AudioFsm() = default;

    AudioState GetState() const { return st_; }

    // eventを入れるとnextとactionsが返る（最大2個）
    AudioTransition Dispatch(AudioFsmEvent event)
    {
        const auto table = kTable[static_cast<uint32_t>(st_)][static_cast<uint32_t>(event)];
        st_              = table.next;
        return table;
    }

    void force_state(AudioState s) { st_ = s; }

private:
    AudioState st_{AudioState::Idle};

    static constexpr AudioTransition kTable[static_cast<uint32_t>(AudioState::Max)][static_cast<uint32_t>(
        AudioFsmEvent::Max)] = {
        // =========================
        // Idle
        // =========================
        {
         /* UiPlayPressed */ {AudioState::Playing, AudioAction::StartPlayback, AudioAction::EmitPlaybackStarted},
         /* UiRecPressed  */ {AudioState::Recording, AudioAction::StartRecord, AudioAction::EmitRecordStarted},
         /* PbEnded       */ {AudioState::Idle, AudioAction::None, AudioAction::None},
         /* RecEnded      */ {AudioState::Idle, AudioAction::None, AudioAction::None},
         },

        // =========================
        // Playing
        // =========================
        {
         /* UiPlayPressed */ {AudioState::Paused, AudioAction::PausePlayback, AudioAction::EmitPlaybackPaused},
         /* UiRecPressed  */ {AudioState::Playing, AudioAction::None, AudioAction::None},
         /* PbEnded       */ {AudioState::Idle, AudioAction::None, AudioAction::EmitPlaybackStopped},
         /* RecEnded      */ {AudioState::Playing, AudioAction::None, AudioAction::None},
         },

        // =========================
        // Paused
        // =========================
        {
         /* UiPlayPressed */ {AudioState::Playing, AudioAction::ResumePlayback, AudioAction::EmitPlaybackResumed},
         /* UiRecPressed  */ {AudioState::Paused, AudioAction::None, AudioAction::None},
         /* PbEnded       */ {AudioState::Idle, AudioAction::None, AudioAction::EmitPlaybackStopped},
         /* RecEnded      */ {AudioState::Paused, AudioAction::None, AudioAction::None},
         },

        // =========================
        // Recording
        // =========================
        {
         /* UiPlayPressed */ {AudioState::Recording, AudioAction::None, AudioAction::None},
         /* UiRecPressed  */ {AudioState::Recording, AudioAction::StopRecord, AudioAction::None},
         /* PbEnded       */ {AudioState::Recording, AudioAction::None, AudioAction::None},
         /* RecEnded      */ {AudioState::Idle, AudioAction::None, AudioAction::EmitRecordStopped},
         },
    };
};

}  // namespace app
}  // namespace core0
