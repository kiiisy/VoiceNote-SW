/**
 * @file ui_input_handler.h
 * @brief UI入力受付とデバウンス処理を扱うハンドラ
 */

#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "play.h"
#include "play_agc.h"
#include "playlist.h"
#include "rec.h"
#include "rec_option.h"

namespace core1 {

namespace gui {
class LvglController;
}

namespace ipc {
class IpcClient;
}

namespace app {

class AppEventBus;
class PlayListHandler;

/**
 * @brief GUIコールバックの受付とUI要求の正規化を担当
 */
class UiInputHandler
{
public:
    void Init(gui::LvglController *gui, ipc::IpcClient *ipc, AppEventBus *event_bus, PlayListHandler *playlist_handler);

    void OnPlayRequeste(const gui::PlayRequest &req);
    void OnRecRequeste(const gui::RecRequest &req);
    void OnPlayAgcRequeste(const gui::PlayAgcRequest &req);
    void OnRecOptionRequeste(const gui::RecOptionRequest &req);
    void OnPlayListRequeste(const gui::PlayListRequest &req);

    bool HasPlayRequest() const { return has_play_req_; }
    bool HasRecRequest() const { return has_rec_req_; }

    const gui::PlayRequest &GetPlayRequest() const { return last_play_req_; }
    const gui::RecRequest  &GetRecRequest() const { return last_rec_req_; }

private:
    static void OnPlayRequestCallback(const gui::PlayRequest &req, void *user);
    static void OnRecRequestCallback(const gui::RecRequest &req, void *user);
    static void OnPlayAgcRequestCallback(const gui::PlayAgcRequest &req, void *user);
    static void OnRecOptionRequestCallback(const gui::RecOptionRequest &req, void *user);
    static void OnPlayListRequestCallback(const gui::PlayListRequest &req, void *user);
    void        BindGuiCallbacks();

    gui::LvglController *gui_{nullptr};
    ipc::IpcClient      *ipc_{nullptr};
    AppEventBus         *event_bus_{nullptr};
    PlayListHandler     *playlist_handler_{nullptr};

    static constexpr uint32_t kUiPlayDebounceMs = 180;
    static constexpr uint32_t kUiRecDebounceMs  = 400;
    uint32_t                  last_ui_play_req_ms_{0};
    uint32_t                  last_ui_rec_req_ms_{0};

    gui::PlayRequest last_play_req_{};
    bool             has_play_req_{false};
    gui::RecRequest  last_rec_req_{};
    bool             has_rec_req_{false};
};

}  // namespace app
}  // namespace core1
