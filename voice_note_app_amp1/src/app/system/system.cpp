/**
 * @file system.cpp
 * @brief Systemの実装
 */
// 自ヘッダー
#include "system.h"

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
 * @brief System依存オブジェクトの初期化とハンドラ接続を行う
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

    prev_ms_       = GetTimeMs();
    next_touch_ms_ = prev_ms_;
    next_lvgl_ms_  = prev_ms_;

    playlist_handler_.Init(ipc_, gui_);
    ui_input_handler_.Init(gui_, ipc_, &event_bus_, &playlist_handler_);
    ipc_status_handler_.Init(ipc_, gui_, &event_bus_);
    ipc_command_handler_.Init(ipc_, &ui_input_handler_);
    ui_view_updater_.Init(gui_);
}

/**
 * @brief 再生要求をUI入力ハンドラへ渡す
 */
void System::OnPlayRequeste(const gui::PlayRequest &req)
{
    ui_input_handler_.OnPlayRequeste(req);
    ProcessPendingEvents();
}

/**
 * @brief 録音要求をUI入力ハンドラへ渡す
 */
void System::OnRecRequeste(const gui::RecRequest &req)
{
    ui_input_handler_.OnRecRequeste(req);
    ProcessPendingEvents();
}

/**
 * @brief 再生AGC要求をUI入力ハンドラへ渡す
 */
void System::OnPlayAgcRequeste(const gui::PlayAgcRequest &req)
{
    ui_input_handler_.OnPlayAgcRequeste(req);
}

/**
 * @brief 録音オプション要求をUI入力ハンドラへ渡す
 */
void System::OnRecOptionRequeste(const gui::RecOptionRequest &req)
{
    ui_input_handler_.OnRecOptionRequeste(req);
}

/**
 * @brief 再生リスト要求をUI入力ハンドラへ渡す
 */
void System::OnPlayListRequeste(const gui::PlayListRequest &req)
{
    ui_input_handler_.OnPlayListRequeste(req);
}

/**
 * @brief イベントキューを消化してFSM dispatchを実行する
 */
void System::ProcessPendingEvents()
{
    Event event{};
    // 1ループで溜まっているイベントを可能な限り消化する
    while (event_bus_.Pop(&event)) {
        const auto table = media_fsm_.Dispatch(event);
        ExecuteFsmAction(table.action1);
        ExecuteFsmAction(table.action2);
    }
}

/**
 * @brief FSMアクションを各ハンドラへ振り分ける
 */
void System::ExecuteFsmAction(ActionId action)
{
    LOGI("Next Action : %d", action);

    if (action == ActionId::None) {
        return;
    }

    // UI操作系のアクションを優先的に処理する
    if (ui_view_updater_.Execute(action)) {
        return;
    }

    // 送信系アクションはIPCコマンドハンドラへ委譲する
    if (ipc_command_handler_.Execute(action)) {
        return;
    }

    if (action == ActionId::LogInvalid) {
        LOGW("Invalid op for state=%u", static_cast<unsigned>(media_fsm_.GetState()));
    }
}

/**
 * @brief アプリのメインループを実行する
 */
void System::Run()
{
    while (1) {
        const uint32_t now_ms = GetTimeMs();

        uint32_t delta_ms = now_ms - prev_ms_;
        prev_ms_          = now_ms;

        // LVGLの時刻は一度に進めすぎない
        if (delta_ms > kMaxDeltaMsPerLoop) {
            delta_ms = kMaxDeltaMsPerLoop;
        }

        if (delta_ms) {
            gui_->TiclInc(delta_ms);
        }

        // IPC受信で積まれたイベントをFSMへ反映する
        ipc_->PollN(kIpcPollBatch);
        ProcessPendingEvents();

        // タッチ更新周期は押下中/非押下で切り替える
        if ((int32_t)(now_ms - next_touch_ms_) >= 0) {
            const uint32_t touch_period_ms = touch_->IsPressed() ? kTouchPeriodMsHot : kTouchPeriodMsIdle;
            next_touch_ms_ += touch_period_ms;

            const auto result = touch_->Run();
            if (result == platform::TouchCtrl::RunResult::NeedRearmIrq && gpio_pl_) {
                gpio_pl_->ReenableIrq();
            }
        }

        // LVGL駆動後に発生したイベントも同一ループで処理する
        if ((int32_t)(now_ms - next_lvgl_ms_) >= 0) {
            next_lvgl_ms_ += lv_period_ms_;
            gui_->TimerHandler();
            ProcessPendingEvents();
        }

        usleep(sleep_us_);
    }
}

}  // namespace app
}  // namespace core1
