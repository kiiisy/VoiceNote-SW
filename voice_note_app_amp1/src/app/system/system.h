/**
 * @file system.h
 * @brief システムの実行ループを統括するランタイム
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "app_event_bus.h"
#include "fsm.h"
#include "ipc_command_handler.h"
#include "ipc_status_handler.h"
#include "playlist_handler.h"
#include "ui_input_handler.h"
#include "ui_view_updater.h"

namespace core1 {

namespace gui {
class LvglController;
struct PlayRequest;
struct RecRequest;
struct PlayAgcRequest;
struct RecOptionRequest;
struct PlayListRequest;
}  // namespace gui

namespace platform {
class TouchCtrl;
class GpioPl;
}  // namespace platform

namespace ipc {
class IpcClient;
}  // namespace ipc

namespace app {

/**
 * @brief システム実行クラス
 */
class System
{
public:
    struct Deps
    {
        ipc::IpcClient      *ipc{nullptr};      // IPCクライアント
        gui::LvglController *gui{nullptr};      // LVGL/画面遷移コントローラ
        platform::TouchCtrl *touch{nullptr};    // タッチ入力制御
        platform::GpioPl    *gpio_pl{nullptr};  // IRQ再許可が必要なら
    };

    System() = default;
    ~System() = default;

    void Init(const Deps &deps);
    void Run();

    void SetLvglPeriodMs(uint32_t ms) { lv_period_ms_ = ms; }
    void SetSleepUs(uint32_t us) { sleep_us_ = us; }
    void OnPlayRequeste(const gui::PlayRequest &req);
    void OnRecRequeste(const gui::RecRequest &req);
    void OnPlayAgcRequeste(const gui::PlayAgcRequest &req);
    void OnRecOptionRequeste(const gui::RecOptionRequest &req);
    void OnPlayListRequeste(const gui::PlayListRequest &req);

private:
    void ProcessPendingEvents();
    void ExecuteFsmAction(ActionId action);

    ipc::IpcClient      *ipc_{nullptr};      // IPCクライアント
    gui::LvglController *gui_{nullptr};      // GUIコントローラ
    platform::TouchCtrl *touch_{nullptr};    // タッチ入力制御
    platform::GpioPl    *gpio_pl_{nullptr};  // PL GPIO

    uint32_t lv_period_ms_{5};  // LVGL timer handlerの呼び出し周期（ms）
    uint32_t sleep_us_{1000};   // メインループsleep値（us）

    static constexpr uint32_t kMaxDeltaMsPerLoop = 50;  // LVGLへ渡す1回あたりの最大経過時間[ms]
    static constexpr uint32_t kIpcPollBatch      = 8;   // 1ループで処理するIPCイベント件数
    static constexpr uint32_t kTouchPeriodMsHot  = 3;   // 押下中のタッチ追従周期[ms]
    static constexpr uint32_t kTouchPeriodMsIdle = 10;  // 非押下時のタッチ確認周期[ms]

    uint32_t prev_ms_{0};        // 前回ループのnow_ms
    uint32_t next_touch_ms_{0};  // 次回タッチ処理予定時刻（ms）
    uint32_t next_lvgl_ms_{0};   // 次回LVGL処理予定時刻（ms）

    AppEventBus      event_bus_{};
    PlayListHandler  playlist_handler_{};
    UiInputHandler   ui_input_handler_{};
    IpcStatusHandler ipc_status_handler_{};
    IpcCommandHandler ipc_command_handler_{};
    UiViewUpdater    ui_view_updater_{};

    Fsm media_fsm_{};
};

}  // namespace app
}  // namespace core1
