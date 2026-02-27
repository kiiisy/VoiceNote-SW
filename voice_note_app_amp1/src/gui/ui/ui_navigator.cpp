// 自ヘッダー
#include "ui_navigator.h"

// 標準ライブラリ
#include <cstdio>
#include <cstring>

// プロジェクトライブラリ
#include "logger_core.h"

namespace core1 {
namespace gui {

UiNavigator::UiNavigator(ScreenStore &store) : store_(store), transitions_(store, *this), bindings_(store, *this) {}

void UiNavigator::Init()
{
    bindings_.Init();
    GoHome();
}

void UiNavigator::GoHome()
{
    current_ = Screen::Home;
    lv_screen_load(store_.GetHomeScreen());
}

void UiNavigator::GoPlayOptions()
{
    current_ = Screen::PlayOptions;
    lv_screen_load(store_.GetPlayOptScreen());
}

void UiNavigator::GoRecOptions()
{
    current_ = Screen::RecOptions;
    lv_screen_load(store_.GetRecOptScreen());
}

void UiNavigator::GoPlay()
{
    current_ = Screen::Play;
    transitions_.HomeToPlay();

    PlayListRequest req{};
    std::snprintf(req.path, sizeof(req.path), "%s", "/");
    if (on_playlistreq_) {
        on_playlistreq_(req, on_playlistreq_user_);
    }
}

void UiNavigator::GoRec()
{
    current_ = Screen::Recorder;
    transitions_.HomeToRec();
}

void UiNavigator::Back()
{
    switch (current_) {
        case Screen::Home:
            // ホームからホームの遷移は起きえないためワーニングを出す
            LOGW("Transitioning from one home screen to another.");
            return;
        case Screen::Recorder:
            transitions_.RecToHome();
            return;
        case Screen::Play:
            transitions_.PlayToHome();
            return;
        case Screen::RecOptions:
            current_ = Screen::Recorder;
            lv_screen_load(store_.GetRecScreen());
            return;
        case Screen::PlayOptions:
            current_ = Screen::Play;
            lv_screen_load(store_.GetPlayScreen());
            return;
    }
}

void UiNavigator::SetPlayAgcRequesteCallback(PlayAgcRequesteFn fn, void *user)
{
    on_playagc_      = fn;
    on_playagc_user_ = user;
}

void UiNavigator::SetRecOptionRequesteCallback(RecOptionRequesteFn fn, void *user)
{
    on_recopt_      = fn;
    on_recopt_user_ = user;
}

void UiNavigator::SetPlayRequesteCallback(PlayRequesteFn fn, void *user)
{
    on_playreq_      = fn;
    on_playreq_user_ = user;
}

void UiNavigator::SetRecRequesteCallback(RecRequesteFn fn, void *user)
{
    on_recmain_      = fn;
    on_recmain_user_ = user;
}

void UiNavigator::SetPlayListRequesteCallback(PlayListRequesteFn fn, void *user)
{
    on_playlistreq_      = fn;
    on_playlistreq_user_ = user;
}

void UiNavigator::NotifyPlayAgcDone(const play_agc_params_t &p)
{
    core1::gui::PlayAgcRequest req{};
    req.dist_link     = p.dist_link;
    req.dist_mm       = p.dist_mm;
    req.min_gain_x100 = p.min_gain_x100;
    req.max_gain_x100 = p.max_gain_x100;
    req.speed_k       = p.speed_k;

    if (on_playagc_) {
        on_playagc_(req, on_playagc_user_);
    }

    current_ = Screen::Play;
    lv_screen_load(store_.GetPlayScreen());
}

void UiNavigator::NotifyRecOptionDone(const rec_option_params_t &p)
{
    core1::gui::RecOptionRequest req{};
    req.dc_enable         = p.dc_enable;
    req.dc_fc_hz          = p.dc_fc_hz;
    req.ng_enable         = p.ng_enable;
    req.ng_th_open_x1000  = p.ng_th_open_x1000;
    req.ng_th_close_x1000 = p.ng_th_close_x1000;
    req.ng_attack_ms      = p.ng_attack_ms;
    req.ng_release_ms     = p.ng_release_ms;

    if (on_recopt_) {
        on_recopt_(req, on_recopt_user_);
    }

    current_ = Screen::Recorder;
    lv_screen_load(store_.GetRecScreen());
}

void UiNavigator::SetPlayFileList(const char *names[kMaxFiles], uint16_t count)
{
    if (count > kMaxFiles) {
        count = kMaxFiles;
    }
    file_count_ = count;

    for (uint16_t i = 0; i < kMaxFiles; ++i) {
        file_names_[i][0] = '\0';
        if (i < count && names[i]) {
            std::snprintf(file_names_[i], sizeof(file_names_[i]), "%s", names[i]);
        }
    }

    const char *view[kMaxFiles]{};
    for (uint16_t i = 0; i < kMaxFiles; ++i) {
        view[i] = file_names_[i];
    }
    SetPlayListItems(&store_.GetPlayUi().file_sheet, view);

    if (file_count_ == 0) {
        current_play_index_ = 0;
        SetPlayListPlaying(&store_.GetPlayUi().file_sheet, -1);
    } else if (current_play_index_ >= file_count_) {
        current_play_index_ = 0;
        SetPlayListPlaying(&store_.GetPlayUi().file_sheet, current_play_index_);
    }
}

void UiNavigator::NotifyPlayFileSelected(uint16_t index)
{
    if (index >= file_count_ || index >= kMaxFiles) {
        return;
    }

    current_play_index_ = index;

    // UIハイライト更新
    SetPlayListPlaying(&store_.GetPlayUi().file_sheet, index);

    // 実再生要求
    NotifyPlayMain();
}

void UiNavigator::NotifyPlayMain()
{
    int16_t ui_index = store_.GetPlayUi().file_sheet.current_playing;
    if (ui_index >= 0 && (uint16_t)ui_index < file_count_) {
        current_play_index_ = (uint16_t)ui_index;
    }

    if (file_count_ == 0 || file_names_[current_play_index_][0] == '\0') {
        LOGW("NotifyPlayMain ignored: no playable file");
        return;
    }

    core1::gui::PlayRequest req{};
    req.track_id = current_play_index_;
    std::snprintf(req.filename, sizeof(req.filename), "%s", file_names_[current_play_index_]);

    if (on_playreq_) {
        on_playreq_(req, on_playreq_user_);
    }
}

void UiNavigator::NotifyRecMain()
{
    gui::RecRequest req{};
    req.bits           = 16;
    req.sample_rate_hz = 48000;
    req.ch             = 2;

    // 一旦固定の名前で設定する
    const char *name = "recording_test.wav";
    std::snprintf(req.filename, sizeof(req.filename), "%s", name);

    if (on_recmain_) {
        on_recmain_(req, on_recmain_user_);
    }
}

void UiNavigator::SetPlayView(bool playing)
{
    using gui::play_view_state_t;
    gui::SetPlayViewState(&store_.GetPlayUi(), playing ? play_view_state_t::ShowPause : play_view_state_t::ShowPlay);
}

void UiNavigator::SetPlaybackProgress(uint32_t position_ms, uint32_t duration_ms)
{
    const char *name = file_names_[current_play_index_][0] ? file_names_[current_play_index_] : "no_file";
    gui::SetPlaySeek(&store_.GetPlayUi(), name, position_ms, duration_ms);
}

void UiNavigator::SetRecordView(bool recording)
{
    using gui::record_view_state_t;
    gui::SetRecordViewState(&store_.GetRecUi(),
                            recording ? record_view_state_t::ShowStop : record_view_state_t::ShowStart);
}

void UiNavigator::SetRecordProgress(uint32_t captured_ms, uint32_t target_ms)
{
    gui::SetRecordSeek(&store_.GetRecUi(), captured_ms, target_ms);
}

void UiNavigator::SetPlaybackUiState(uint8_t state)
{
    if (!store_.GetPlayUi().label_main) {
        return;
    }
    lv_label_set_text(store_.GetPlayUi().label_main, (state == 1) ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

void UiNavigator::SetRecordUiState(uint8_t state)
{
    if (!store_.GetRecUi().label_main) {
        return;
    }
    lv_label_set_text(store_.GetRecUi().label_main, (state == 1) ? LV_SYMBOL_STOP : LV_SYMBOL_PLAY);
}

}  // namespace gui
}  // namespace core1
