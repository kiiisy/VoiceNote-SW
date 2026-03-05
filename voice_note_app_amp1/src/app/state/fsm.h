/**
 * @file fsm.h
 * @brief 状態遷移の管理
 */
#pragma once

// 標準ライブラリ
#include <cstddef>
#include <cstdint>

namespace core1 {
namespace app {

// 状態
enum class State : uint8_t
{
    Idle = 0,
    Playing,
    Paused,
    Recording,

    Max
};

// イベント
enum class Event : uint8_t
{
    UiPlayPressed = 0,
    UiRecPressed,
    IpcPlaybackStopped,
    IpcRecordStopped,

    Max
};

// 外部向けのアクションと内部向けのアクションを分けた方がいいが、面倒なので同じenumで管理
enum class ActionId : uint16_t
{
    None = 0,

    // 外部向け
    SendPlay,
    SendPause,
    SendResume,
    SendRecStart,
    SendRecStop,

    // 内部向け
    UiPlayShowPlayIcon,   // ▶
    UiPlayShowPauseIcon,  // ⏸
    UiRecShowPlayIcon,    // ▶
    UiRecShowStopIcon,    // ■

    LogInvalid
};

struct Transition
{
    State    next;
    ActionId action1;
    ActionId action2;
};

class Fsm
{
public:
    Fsm() : st_(State::Idle) {}

    State GetState() const { return st_; }

    // eventを入れるとnextとactionsが返る（最大2個）
    Transition Dispatch(Event event)
    {
        const auto table = kTable[static_cast<uint32_t>(st_)][static_cast<uint32_t>(event)];
        st_              = table.next;
        return table;
    }

private:
    State st_{State::Idle};

    static constexpr Transition kTable[static_cast<uint32_t>(State::Max)][static_cast<uint32_t>(Event::Max)] = {
        // =========================================================
        // Idle
        // =========================================================
        {
         /* UiPlayPressed       */ {State::Playing, ActionId::SendPlay, ActionId::UiPlayShowPauseIcon},
         /* UiRecPressed        */ {State::Recording, ActionId::SendRecStart, ActionId::UiRecShowStopIcon},
         /* IpcPlaybackStopped  */ {State::Idle, ActionId::UiPlayShowPlayIcon, ActionId::None},
         /* IpcRecordStopped    */ {State::Idle, ActionId::UiRecShowPlayIcon, ActionId::None},
         },

        // =========================================================
        // Playing
        // =========================================================
        {
         /* UiPlayPressed       */ {State::Paused, ActionId::SendPause, ActionId::UiPlayShowPlayIcon},
         /* UiRecPressed        */ {State::Playing, ActionId::LogInvalid, ActionId::None},
         /* IpcPlaybackStopped  */ {State::Idle, ActionId::UiPlayShowPlayIcon, ActionId::None},
         /* IpcRecordStopped    */ {State::Playing, ActionId::None, ActionId::None},
         },

        // =========================================================
        // Paused
        // =========================================================
        {
         /* UiPlayPressed       */ {State::Playing, ActionId::SendResume, ActionId::UiPlayShowPauseIcon},
         /* UiRecPressed        */ {State::Paused, ActionId::LogInvalid, ActionId::None},
         /* IpcPlaybackStopped  */ {State::Idle, ActionId::UiPlayShowPlayIcon, ActionId::None},
         /* IpcRecordStopped    */ {State::Paused, ActionId::None, ActionId::None},
         },

        // =========================================================
        // Recording
        // =========================================================
        {
         /* UiPlayPressed       */ {State::Recording, ActionId::LogInvalid, ActionId::None},
         /* UiRecPressed        */ {State::Idle, ActionId::SendRecStop, ActionId::UiRecShowPlayIcon},
         /* IpcPlaybackStopped  */ {State::Recording, ActionId::None, ActionId::None},
         /* IpcRecordStopped    */ {State::Idle, ActionId::UiRecShowPlayIcon, ActionId::None},
         },
    };
};

}  // namespace app
}  // namespace core1
