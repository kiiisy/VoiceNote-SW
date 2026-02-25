/**
 * @file system.cpp
 * @brief Systemの実装
 */
// 自ヘッダー
#include "system.h"

// 標準ライブラリ
#include <cstdio>
#include <cstring>

// Vitisライブラリ
#include "sleep.h"

// プロジェクトライブラリ
#include "app_client.h"
#include "gpio_pl.h"
#include "logger_core.h"
#include "lvgl_controller.h"
#include "touch_ctrl.h"
#include "utility.h"

namespace core1 {
namespace app {

/**
 * @brief 依存の保存、周期制御の初期化、GUIからのコールバック接続を行う
 */
void System::Init(const Deps &deps)
{
    ipc_     = deps.ipc;
    gui_     = deps.gui;
    touch_   = deps.touch;
    gpio_pl_ = deps.gpio_pl;

    if (!ipc_ || !gui_ || !touch_) {
        LOGE("System deps invalid: ipc=%p gui=%p touch=%p", ipc_, gui_, touch_);
    }

    prev_ms_    = GetTimeMs();
    next_touch_ = prev_ms_;
    next_lv_    = prev_ms_;

    // GUIからのイベントを受け取る
    gui_->SetPlayCallback(
        [](const core1::gui::PlayRequest &req, void *user) {
            auto *self = static_cast<System *>(user);
            self->OnPlayRequeste(req);
        },
        this);

    gui_->SetRecRequesteCallback(
        [](const core1::gui::RecRequest &req, void *user) {
            auto *self = static_cast<System *>(user);
            self->OnRecRequeste(req);
        },
        this);

    gui_->SetPlayAgcCallback(
        [](const core1::gui::PlayAgcRequest &opt, void *user) {
            auto *self = static_cast<System *>(user);
            self->OnPlayAgcRequeste(opt);
        },
        this);

    gui_->SetPlayListRequesteCallback(
        [](const core1::gui::PlayListRequest &req, void *user) {
            auto *self = static_cast<System *>(user);
            self->OnPlayListRequeste(req);
        },
        this);

    // IPCからのイベントを受け取る
    ipc_->on_playback_status = [&](const core::ipc::PlaybackStatusPayload &st) {
        gui_->SetPlaybackUiState(st.state);
        gui_->SetPlaybackProgress(st.position_ms, st.duration_ms);

        const uint8_t prev = last_pb_state_;
        last_pb_state_     = st.state;

        if (prev != st.state && st.state == 0) {
            HandleEvent(Event::IpcPlaybackStopped);
        }
    };

    ipc_->on_record_status = [&](const core::ipc::RecordStatusPayload &st) {
        gui_->SetRecordUiState(st.state);
        gui_->SetRecordProgress(st.captured_ms, st.target_ms);
        if (st.state == 0) {
            HandleEvent(Event::IpcRecordStopped);
        }
    };

    auto prev_on_dir_entry = ipc_->on_dir_entry;
    ipc_->on_dir_entry     = [this, prev_on_dir_entry](uint32_t seq, uint16_t index, uint16_t total_hint,
                                                   const core::ipc::DirEntryPayload &e) {
        if (prev_on_dir_entry) {
            prev_on_dir_entry(seq, index, total_hint, e);
        }

        if (!list_dir_inflight_ || seq != list_dir_seq_) {
            return;
        }
        if (list_dir_count_ >= kMaxPlayListFiles) {
            return;
        }
        if (e.attr != 0) {
            return;
        }

        std::strncpy(list_dir_names_[list_dir_count_], e.name, sizeof(list_dir_names_[list_dir_count_]) - 1);
        list_dir_names_[list_dir_count_][sizeof(list_dir_names_[list_dir_count_]) - 1] = '\0';
        ++list_dir_count_;
    };

    auto prev_on_ack = ipc_->on_ack;
    ipc_->on_ack     = [this, prev_on_ack](core::ipc::CmdId cmd, uint32_t seq, core::ipc::AckStatus st, int32_t rc) {
        if (prev_on_ack) {
            prev_on_ack(cmd, seq, st, rc);
        }

        if (cmd != core::ipc::CmdId::ListDir || seq != list_dir_seq_ || st != core::ipc::AckStatus::Done) {
            return;
        }

        list_dir_inflight_ = false;
        if (rc < 0) {
            list_dir_count_ = 0;
        }

        for (uint16_t i = 0; i < kMaxPlayListFiles; ++i) {
            list_dir_name_ptrs_[i] = list_dir_names_[i];
        }
        gui_->SetPlayFileList(list_dir_name_ptrs_, list_dir_count_);
    };
}

/**
 * @brief GUIからの再生要求を送る
 */
void System::OnPlayRequeste(const gui::PlayRequest &req)
{
    const uint32_t now = GetTimeMs();
    if ((now - last_ui_play_req_ms_) < kUiPlayDebounceMs) {
        LOGW("Ignore duplicated UiPlayPressed within debounce window");
        return;
    }
    last_ui_play_req_ms_ = now;

    last_play_req_ = req;
    has_play_req_  = true;
    HandleEvent(Event::UiPlayPressed);
}

/**
 * @brief GUIからの録音要求を送る
 */
void System::OnRecRequeste(const gui::RecRequest &req)
{
    last_rec_req_ = req;
    has_rec_req_  = true;
    HandleEvent(Event::UiRecPressed);
}

/**
 * @brief GUIからのAGC要求を送る
 */
void System::OnPlayAgcRequeste(const gui::PlayAgcRequest &req)
{
    last_playopt_req_ = req;
    has_playopt_req_  = true;

    if (!ipc_) {
        return;
    }

    ipc_->SetAgc(req.dist_link, req.dist_mm, req.min_gain_x100, req.max_gain_x100, (int16_t)req.speed_k);
}

void System::OnPlayListRequeste(const gui::PlayListRequest &req)
{
    if (!ipc_) {
        return;
    }

    list_dir_count_    = 0;
    list_dir_inflight_ = true;
    for (uint16_t i = 0; i < kMaxPlayListFiles; ++i) {
        list_dir_names_[i][0]  = '\0';
        list_dir_name_ptrs_[i] = list_dir_names_[i];
    }

    list_dir_seq_ = ipc_->ListDir(req.path);
}

void System::HandleEvent(Event event)
{
    const auto t = media_fsm_.Dispatch(event);

    // act1/act2 を順に実行（None は no-op）
    ExecuteAction(t.act1);
    ExecuteAction(t.act2);
}

void System::ExecuteAction(ActionId action)
{
    LOGI("Next Action : %d", action);

    switch (action) {
        case ActionId::None:
            return;

        // UI関連のアクション
        case ActionId::UiPlayShowPlayIcon:
            // 再生アイコンはIPCのPlaybackStatus(state)のみで更新する。
            // ここで先行反映すると、状態反映の往復で一瞬だけ戻ることがある。
            return;

        case ActionId::UiPlayShowPauseIcon:
            // 再生アイコンはIPCのPlaybackStatus(state)のみで更新する。
            return;

        case ActionId::UiRecShowPlayIcon:
            if (gui_) {
                gui_->SetRecordView(false);  // ▶ 表示
            }
            return;

        case ActionId::UiRecShowStopIcon:
            if (gui_) {
                gui_->SetRecordView(true);  // ■ 表示
            }
            return;

        // IPC関連のアクション
        case ActionId::SendPlay:
            if (!ipc_) {
                return;
            }
            if (!has_play_req_) {
                LOGW("SendPlay without cached PlayRequest");
                return;
            }
            LOGI("Play: track_id=%u filename=%s", (unsigned)last_play_req_.track_id, last_play_req_.filename);
            ipc_->Play(last_play_req_.filename);
            return;

        case ActionId::SendPause:
            ipc_->Pause();
            return;

        case ActionId::SendResume:
            ipc_->Resume();
            return;

        case ActionId::SendRecStart:
            if (!ipc_) {
                return;
            }
            if (!has_rec_req_) {
                LOGW("SendRecStart without cached RecordRequest");
                return;
            }
            ipc_->RecStart(last_rec_req_.filename, last_rec_req_.sample_rate_hz, last_rec_req_.bits, last_rec_req_.ch);
            return;

        case ActionId::SendRecStop:
            ipc_->RecStop();
            return;

        case ActionId::LogInvalid:
            LOGW("Invalid op for state=%u", static_cast<unsigned>(media_fsm_.GetState()));
            return;

        default:
            return;
    }
}

/**
 * @brief メインループ
 */
void System::Run()
{
    while (1) {
        const uint32_t now = GetTimeMs();

        uint32_t dt = now - prev_ms_;
        prev_ms_    = now;

        if (dt > 50)
            dt = 50;

        if (dt) {
            gui_->TiclInc(dt);
        }

        // ポーリング
        ipc_->PollN(8);

        // タッチ読み取り
        const uint32_t touch_period = touch_->IsPressed() ? 5u : 15u;
        if ((int32_t)(now - next_touch_) >= 0) {
            next_touch_ += touch_period;
            touch_->Run();
        }

        // ここで持ちたくない
        if (gpio_pl_) {
            gpio_pl_->EnableIrqLight();
        }

        // LVGL
        if ((int32_t)(now - next_lv_) >= 0) {
            next_lv_ += lv_period_ms_;
            gui_->TimerHandler();
        }

        usleep(sleep_us_);
    }
}

}  // namespace app
}  // namespace core1
