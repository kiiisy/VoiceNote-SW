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
#include "screen_store.h"
#include "ui_bindings.h"
#include "ui_transitions.h"

namespace core1 {
namespace gui {

class UiNavigator
{
public:
    using PlayRequesteFn    = void (*)(const core1::gui::PlayRequest &req, void *user);
    using RecRequesteFn     = void (*)(const core1::gui::RecRequest &req, void *user);
    using PlayAgcRequesteFn = void (*)(const core1::gui::PlayAgcRequest &opt, void *user);
    using PlayListRequesteFn = void (*)(const core1::gui::PlayListRequest &req, void *user);

    static constexpr uint16_t kMaxFiles = 10;

    enum class Screen : uint8_t
    {
        Home,
        Recorder,
        Play,
        PlayOptions,
    };

    explicit UiNavigator(ScreenStore &store);
    UiNavigator() = delete;

    void Init();

    void GoHome();
    void GoRec();
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
    void SetPlayRequesteCallback(PlayRequesteFn fn, void *user);
    void SetRecRequesteCallback(RecRequesteFn fn, void *user);
    void SetPlayListRequesteCallback(PlayListRequesteFn fn, void *user);

    void NotifyPlayMain();
    void NotifyRecMain();
    void NotifyPlayAgcDone(const play_agc_params_t &p);

    void SetPlayFileList(const char *names[kMaxFiles], uint16_t count);
    void NotifyPlayFileSelected(uint16_t index);

    Screen GetCurrent() const { return current_; }
    void   SetCurrent(Screen s) { current_ = s; }

    ScreenStore &store() { return store_; }

private:
    ScreenStore &store_;
    Screen       current_{Screen::Home};

    UiTransitions transitions_;
    UiBindings    bindings_;

    PlayAgcRequesteFn on_playagc_{nullptr};
    void             *on_playagc_user_{nullptr};

    PlayRequesteFn on_playreq_{nullptr};
    void          *on_playreq_user_{nullptr};

    RecRequesteFn on_recmain_{nullptr};
    void         *on_recmain_user_{nullptr};

    PlayListRequesteFn on_playlistreq_{nullptr};
    void              *on_playlistreq_user_{nullptr};

    uint16_t    current_play_index_{0};
    uint16_t    file_count_{0};
    char        file_names_[kMaxFiles][80]{};
};

}  // namespace gui
}  // namespace core1
