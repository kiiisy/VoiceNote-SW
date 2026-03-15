#pragma once

// 標準ライブラリ
#include <cstdint>

// LVGLライブラリ
#include "lvgl.h"

// プロジェクトライブラリ
#include "play.h"
#include "play_agc.h"
#include "playlist.h"
#include "rec.h"
#include "rec_option.h"
#include "screen_store.h"
#include "ui_bindings.h"
#include "ui_transitions.h"

namespace core1 {
namespace gui {

class UiNavigator
{
public:
    using PlayRequesteFn      = void (*)(const core1::gui::PlayRequest &req, void *user);
    using RecRequesteFn       = void (*)(const core1::gui::RecRequest &req, void *user);
    using PlayAgcRequesteFn   = void (*)(const core1::gui::PlayAgcRequest &opt, void *user);
    using RecOptionRequesteFn = void (*)(const core1::gui::RecOptionRequest &opt, void *user);
    using PlayListRequesteFn  = void (*)(const core1::gui::PlayListRequest &req, void *user);

    static constexpr uint16_t kMaxFiles = 10;

    enum class Screen : uint8_t
    {
        Home,
        Recorder,
        RecOptions,
        Play,
        PlayOptions,
    };

    explicit UiNavigator(ScreenStore &store);
    UiNavigator() = delete;

    void Init();

    void GoHome();
    void GoRec();
    void GoRecOptions();
    void GoPlay();
    void GoPlayOptions();
    void Back();

    void SetPlayView(bool playing);
    void SetPlaybackProgress(uint32_t position_ms, uint32_t duration_ms);
    void SetRecordView(bool recording);
    void SetRecordProgress(uint32_t captured_ms, uint32_t target_ms);
    void SetPlaybackUiState(uint8_t state);
    void SetRecordUiState(uint8_t state);

    void SetPlayAgcRequesteCallback(PlayAgcRequesteFn fn, void *user);
    void SetRecOptionRequesteCallback(RecOptionRequesteFn fn, void *user);
    void SetPlayRequesteCallback(PlayRequesteFn fn, void *user);
    void SetRecRequesteCallback(RecRequesteFn fn, void *user);
    void SetPlayListRequesteCallback(PlayListRequesteFn fn, void *user);

    void NotifyPlayMain();
    void NotifyRecMain();
    void NotifyPlayAgcDone(const play_agc_params_t &p);
    void NotifyRecOptionDone(const rec_option_params_t &p);

    void SetPlayFileList(const char *names[kMaxFiles], uint16_t count);
    void NotifyPlayFileSelected(uint16_t index);

    Screen GetCurrent() const { return current_; }
    void   SetCurrent(Screen s) { current_ = s; }

    ScreenStore &store() { return store_; }

private:
    ScreenStore &store_;                  // 画面インスタンス保管への参照
    Screen       current_{Screen::Home};  // 現在表示中の画面種別

    UiTransitions transitions_;  // 画面遷移の実処理
    UiBindings    bindings_;     // 画面イベントのバインド管理

    PlayAgcRequesteFn   on_playagc_{nullptr};       // AGC設定完了通知のコールバック
    void               *on_playagc_user_{nullptr};  // AGC通知用ユーザデータ
    RecOptionRequesteFn on_recopt_{nullptr};        // 録音オプション完了通知のコールバック
    void               *on_recopt_user_{nullptr};   // 録音オプション通知用ユーザデータ

    PlayRequesteFn on_playreq_{nullptr};       // 再生要求通知のコールバック
    void          *on_playreq_user_{nullptr};  // 再生要求通知用ユーザデータ

    RecRequesteFn on_recmain_{nullptr};       // 録音要求通知のコールバック
    void         *on_recmain_user_{nullptr};  // 録音要求通知用ユーザデータ

    PlayListRequesteFn on_playlistreq_{nullptr};       // 再生リスト要求通知のコールバック
    void              *on_playlistreq_user_{nullptr};  // 再生リスト要求通知用ユーザデータ

    uint16_t current_play_index_{0};        // 現在選択中の再生リストindex
    uint16_t file_count_{0};                // 保持中のファイル数
    char     file_names_[kMaxFiles][80]{};  // 再生要求に使う実ファイル名（/ と拡張子あり）
    char     file_labels_[kMaxFiles][80]{}; // 再生リスト表示用の見た目名
};

}  // namespace gui
}  // namespace core1
