/**
 * @file system.h
 * @brief システムの実行ループを統括するランタイム
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "fsm.h"
#include "play.h"
#include "play_agc.h"
#include "playlist.h"
#include "rec.h"
#include "rec_option.h"

namespace core1 {

namespace gui {
class LvglController;
}

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
    void BindGuiCallbacks();
    void BindIpcCallbacks();

    void HandleEvent(Event event);
    void ExecuteAction(ActionId action);

    ipc::IpcClient      *ipc_{nullptr};      // IPCクライアント
    gui::LvglController *gui_{nullptr};      // GUIコントローラ
    platform::TouchCtrl *touch_{nullptr};    // タッチ入力制御
    platform::GpioPl    *gpio_pl_{nullptr};  // PL GPIO

    uint8_t last_pb_state_{0xFF};
    uint8_t last_rec_state_{0xFF};

    uint32_t                  lv_period_ms_{5};  // LVGL timer handlerの呼び出し周期（ms）
    uint32_t                  sleep_us_{1000};   // メインループsleep値（us）
    static constexpr uint32_t kUiPlayDebounceMs = 180;

    uint32_t prev_ms_{0};     // 前回ループのnow_ms
    uint32_t next_touch_{0};  // 次回タッチ処理予定時刻（ms）
    uint32_t next_lv_{0};     // 次回LVGL処理予定時刻（ms）
    uint32_t last_ui_play_req_ms_{0};

    gui::PlayRequest    last_play_req_{};
    bool                has_play_req_{false};
    gui::PlayAgcRequest last_playopt_req_{};
    bool                has_playopt_req_{false};
    gui::RecRequest     last_rec_req_{};
    bool                has_rec_req_{false};

    static constexpr uint16_t kMaxPlayListFiles = 10;
    uint32_t                  list_dir_seq_{0};
    bool                      list_dir_inflight_{false};
    uint16_t                  list_dir_count_{0};
    char                      list_dir_names_[kMaxPlayListFiles][80]{};
    const char               *list_dir_name_ptrs_[kMaxPlayListFiles]{};

    Fsm media_fsm_{};
};

}  // namespace app
}  // namespace core1
