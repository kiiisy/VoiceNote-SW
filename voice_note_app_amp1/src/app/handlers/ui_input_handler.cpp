/**
 * @file ui_input_handler.cpp
 * @brief UiInputHandlerの実装
 */

// 自ヘッダー
#include "ui_input_handler.h"

// プロジェクトライブラリ
#include "app_client.h"
#include "app_event_bus.h"
#include "logger_core.h"
#include "lvgl_controller.h"
#include "playlist_handler.h"
#include "utility.h"

namespace core1 {
namespace app {

/**
 * @brief 依存オブジェクトを設定してGUIコールバックを接続する
 */
void UiInputHandler::Init(gui::LvglController *gui, ipc::IpcClient *ipc, AppEventBus *event_bus,
                          PlayListHandler *playlist_handler)
{
    gui_              = gui;
    ipc_              = ipc;
    event_bus_        = event_bus;
    playlist_handler_ = playlist_handler;

    BindGuiCallbacks();
}

/**
 * @brief 再生要求を受け取りデバウンス後にFSMイベント化する
 */
void UiInputHandler::OnPlayRequeste(const gui::PlayRequest &req)
{
    const uint32_t now = GetTimeMs();
    // UI連打による重複イベントを抑止する
    if ((now - last_ui_play_req_ms_) < kUiPlayDebounceMs) {
        LOGW("Ignore duplicated UiPlayPressed within debounce window");
        return;
    }

    last_ui_play_req_ms_ = now;

    last_play_req_ = req;
    has_play_req_  = true;

    if (event_bus_) {
        // 実際の遷移判定はSystem側FSMに一本化する
        event_bus_->Push(Event::UiPlayPressed);
    }
}

/**
 * @brief 録音要求を受け取りデバウンス後にFSMイベント化する
 */
void UiInputHandler::OnRecRequeste(const gui::RecRequest &req)
{
    const uint32_t now = GetTimeMs();
    // UI連打による重複イベントを抑止する
    if ((now - last_ui_rec_req_ms_) < kUiRecDebounceMs) {
        LOGW("Ignore duplicated UiRecPressed within debounce window");
        return;
    }

    last_ui_rec_req_ms_ = now;

    last_rec_req_ = req;
    has_rec_req_  = true;

    if (event_bus_) {
        // 実際の遷移判定はSystem側FSMに一本化する
        event_bus_->Push(Event::UiRecPressed);
    }
}

/**
 * @brief 再生AGC設定要求をIPCへ送る
 */
void UiInputHandler::OnPlayAgcRequeste(const gui::PlayAgcRequest &req)
{
    if (!ipc_) {
        return;
    }

    ipc_->SetAgc(req.dist_link, req.dist_mm, req.min_gain_x100, req.max_gain_x100, static_cast<int16_t>(req.speed_k));
}

/**
 * @brief 録音オプション設定要求をIPCへ送る
 */
void UiInputHandler::OnRecOptionRequeste(const gui::RecOptionRequest &req)
{
    if (!ipc_) {
        return;
    }

    // UI値をIPC仕様の固定小数点へ変換する
    const int32_t dc_fc_q16       = static_cast<int32_t>(req.dc_fc_hz) << 16;
    const int32_t ng_th_open_q15  = static_cast<int32_t>(req.ng_th_open_x1000) * 32768 / 1000;
    const int32_t ng_th_close_q15 = static_cast<int32_t>(req.ng_th_close_x1000) * 32768 / 1000;
    ipc_->SetRecOption(req.dc_enable, dc_fc_q16, req.ng_enable, ng_th_open_q15, ng_th_close_q15, req.ng_attack_ms,
                       req.ng_release_ms, req.arec_enable, req.arec_threshold, req.arec_window_shift,
                       req.arec_pretrig_samples, req.arec_required_windows);
}

/**
 * @brief 再生リスト要求をPlayListHandlerへ委譲する
 */
void UiInputHandler::OnPlayListRequeste(const gui::PlayListRequest &req)
{
    if (!playlist_handler_) {
        return;
    }

    playlist_handler_->Request(req);
}

/**
 * @brief GUI再生要求コールバックの中継
 */
void UiInputHandler::OnPlayRequestCallback(const gui::PlayRequest &req, void *user)
{
    auto *self = static_cast<UiInputHandler *>(user);
    if (self) {
        self->OnPlayRequeste(req);
    }
}

/**
 * @brief GUI録音要求コールバックの中継
 */
void UiInputHandler::OnRecRequestCallback(const gui::RecRequest &req, void *user)
{
    auto *self = static_cast<UiInputHandler *>(user);
    if (self) {
        self->OnRecRequeste(req);
    }
}

/**
 * @brief GUI再生AGC要求コールバックの中継
 */
void UiInputHandler::OnPlayAgcRequestCallback(const gui::PlayAgcRequest &req, void *user)
{
    auto *self = static_cast<UiInputHandler *>(user);
    if (self) {
        self->OnPlayAgcRequeste(req);
    }
}

/**
 * @brief GUI録音オプション要求コールバックの中継
 */
void UiInputHandler::OnRecOptionRequestCallback(const gui::RecOptionRequest &req, void *user)
{
    auto *self = static_cast<UiInputHandler *>(user);
    if (self) {
        self->OnRecOptionRequeste(req);
    }
}

/**
 * @brief GUI再生リスト要求コールバックの中継
 */
void UiInputHandler::OnPlayListRequestCallback(const gui::PlayListRequest &req, void *user)
{
    auto *self = static_cast<UiInputHandler *>(user);
    if (self) {
        self->OnPlayListRequeste(req);
    }
}

/**
 * @brief GUIの各入力コールバックを登録する
 */
void UiInputHandler::BindGuiCallbacks()
{
    if (!gui_) {
        return;
    }

    gui_->SetPlayCallback(&UiInputHandler::OnPlayRequestCallback, this);
    gui_->SetRecRequesteCallback(&UiInputHandler::OnRecRequestCallback, this);
    gui_->SetPlayAgcCallback(&UiInputHandler::OnPlayAgcRequestCallback, this);
    gui_->SetRecOptionCallback(&UiInputHandler::OnRecOptionRequestCallback, this);
    gui_->SetPlayListRequesteCallback(&UiInputHandler::OnPlayListRequestCallback, this);
}

}  // namespace app
}  // namespace core1
