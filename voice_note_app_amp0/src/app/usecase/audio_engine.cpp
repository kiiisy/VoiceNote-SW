/**
 * @file audio_engine.cpp
 * @brief audio_engineの実装
 */

// 自ヘッダー
#include "audio_engine.h"

// 標準ライブラリ
#include <algorithm>
#include <cmath>
#include <string.h>

// Vtiisライブラリ
#include "sleep.h"
#include "xil_mmu.h"
#include "xparameters.h"

// プロジェクトライブラリ
#include "acu_core.h"
#include "agc_core.h"
#include "logger_core.h"

namespace core0 {
namespace app {

/**
 * @brief AudioEngineの初期化を行う
 *
 * @retval true  初期化成功
 * @retval false 初期化失敗
 */
bool AudioEngine::Init()
{
    LOG_SCOPE();

    notification_queue_.Reset();
    fsm_        = AudioFsm{};
    fsm_params_ = {};

    if (!pipeline_.Init(ddr_base_addr_, codec_, platform::Acu::GetInstance(), XPAR_XAUDIO_CLEAN_UP_0_BASEADDR)) {
        LOGE("pipeline_.Init failed");
        return false;
    }

    tx_ = pipeline_.tx();
    rx_ = pipeline_.rx();

    // 送受信用プールを準備する
    if (!tx_pool_.Init(ddr_base_addr_, module::AudioBppPool::kBppSize, module::AudioBppPool::kBppsPerChunk)) {
        LOGE("Failed to allocate memory pool on the sending side");
        return false;
    }
    if (!rx_pool_.Init(ddr_base_addr_, module::AudioBppPool::kBppSize, module::AudioBppPool::kBppsPerChunk)) {
        LOGE("Failed to allocate memory pool on the receiving side");
        return false;
    }

    pb_ctrl_.Bind(*tx_, tx_pool_);
    rec_ctrl_.Bind(*rx_, rx_pool_);

    event_pending_ = false;

    return true;
}

/**
 * @brief AudioEngineの終了処理を行う
 */
void AudioEngine::Deinit()
{
    // HW
    pb_ctrl_.Reset();
    rec_ctrl_.Reset();
    pipeline_.Deinit();

    // pools
    tx_pool_.Deinit();
    rx_pool_.Deinit();

    notification_queue_.Reset();
    event_pending_ = false;
    fsm_params_    = {};
}

/**
 * @brief 再生要求を受け付ける
 *
 * @param[in] wav_path 再生対象WAVパス
 * @retval true  要求受理
 * @retval false 引数不正
 */
bool AudioEngine::RequestPlay(const char *wav_path)
{
    if (!wav_path) {
        return false;
    }

    strncpy(play_path_, wav_path, kPathMax - 1);
    play_path_[kPathMax - 1] = '\0';

    fsm_params_.play_path = play_path_;
    PushFsmEvent(AudioFsmEvent::UiPlayPressed);

    return true;
}

/**
 * @brief 録音要求を受け付ける
 *
 * @param[in] wav_path 録音出力WAVパス
 * @param[in] sample_rate_hz サンプルレート
 * @param[in] bits ビット深度
 * @param[in] ch チャネル数
 * @retval true  要求受理
 * @retval false 引数不正
 */
bool AudioEngine::RequestRecord(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch)
{
    if (!wav_path) {
        return false;
    }

    strncpy(rec_path_, wav_path, kPathMax - 1);
    rec_path_[kPathMax - 1] = '\0';

    fsm_params_.rec_path       = rec_path_;
    fsm_params_.sample_rate_hz = sample_rate_hz;
    fsm_params_.bits           = bits;
    fsm_params_.ch             = ch;

    PushFsmEvent(AudioFsmEvent::UiRecPressed);

    return true;
}

/**
 * @brief 録音停止要求を受け付ける
 *
 * @retval true 常にtrue
 */
bool AudioEngine::RequestRecordStop()
{
    if (fsm_.GetState() == AudioState::Recording) {
        PushFsmEvent(AudioFsmEvent::UiRecPressed);
    }

    return true;
}

/**
 * @brief 一時停止要求を受け付ける
 */
void AudioEngine::RequestPause()
{
    if (fsm_.GetState() == AudioState::Playing) {
        PushFsmEvent(AudioFsmEvent::UiPlayPressed);
    }
}

/**
 * @brief 再開要求を受け付ける
 */
void AudioEngine::RequestResume()
{
    if (fsm_.GetState() == AudioState::Paused) {
        PushFsmEvent(AudioFsmEvent::UiPlayPressed);
    }
}

/**
 * @brief 周期処理を1回進める
 */
void AudioEngine::Task()
{
    // FSMイベントがあれば dispatch → action実行
    RunFsm();

    // 状態に応じて駆動
    switch (fsm_.GetState()) {
        case AudioState::Playing:
        case AudioState::Paused:
            ProcessPlayback();
            break;

        case AudioState::Recording:
            ProcessRecord();
            break;

        case AudioState::Idle:
        default:
            // idleは何もしない
            break;
    }

}

/**
 * @brief 再生系の周期処理を実行する
 */
void AudioEngine::ProcessPlayback()
{
    if (pb_ctrl_.IsActive()) {
        pb_ctrl_.Process();
    }

    PlaybackController::EventInfo pev{};
    while (pb_ctrl_.PopEvent(&pev)) {
        switch (pev.type) {
            case PlaybackController::Event::kStarted:
                PublishNotification(AudioNotification::Type::kPlaybackStarted, static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kPaused:
                PublishNotification(AudioNotification::Type::kPlaybackPaused, static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kResumed:
                PublishNotification(AudioNotification::Type::kPlaybackResumed, static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kStopped:
                PublishNotification(AudioNotification::Type::kPlaybackStopped, static_cast<int32_t>(Error::kNone));
                i2s_tx_.Disable();
                PushFsmEvent(AudioFsmEvent::PbEnded);
                break;
            case PlaybackController::Event::kError:
                PublishNotification(AudioNotification::Type::kError, pev.err);
                i2s_tx_.Disable();
                PushFsmEvent(AudioFsmEvent::PbEnded);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief 録音系の周期処理を実行する
 */
void AudioEngine::ProcessRecord()
{
    if (rec_ctrl_.IsActive()) {
        rec_ctrl_.Process();
    }

    RecordController::EventInfo rev{};
    while (rec_ctrl_.PopEvent(&rev)) {
        switch (rev.type) {
            case RecordController::Event::kStarted:
                PublishNotification(AudioNotification::Type::kRecordStarted, static_cast<int32_t>(Error::kNone));
                break;
            case RecordController::Event::kStopped:
                PublishNotification(AudioNotification::Type::kRecordStopped, static_cast<int32_t>(Error::kNone));
                i2s_rx_.Disable();
                PushFsmEvent(AudioFsmEvent::RecEnded);
                break;
            case RecordController::Event::kError:
                PublishNotification(AudioNotification::Type::kError, rev.err);
                PushFsmEvent(AudioFsmEvent::RecEnded);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief 保留中FSMイベントを処理しアクションを実行する
 */
void AudioEngine::RunFsm()
{
    if (event_pending_) {
        event_pending_ = false;
        // 1イベントに対して最大2アクションを順に実行する
        const auto transition = fsm_.Dispatch(event_);
        ExecuteAction(transition.a1);
        ExecuteAction(transition.a2);
    }
}

/**
 * @brief FSMアクションを実行する
 *
 * @param[in] action 実行するアクション
 */
void AudioEngine::ExecuteAction(AudioAction action)
{
    switch (action) {
        case AudioAction::None: {
            return;
        }

        case AudioAction::StartPlayback: {
            i2s_tx_.Enable();

            bool result = pb_ctrl_.Start(fsm_params_.play_path);

            if (!result) {
                i2s_tx_.Disable();
                PublishNotification(AudioNotification::Type::kError, static_cast<int32_t>(Error::kPlaybackStartFailed));
                PushFsmEvent(AudioFsmEvent::PbEnded);
            }
            return;
        }

        case AudioAction::PausePlayback: {
            pb_ctrl_.RequestPause();
            return;
        }

        case AudioAction::ResumePlayback: {
            pb_ctrl_.RequestResume();
            return;
        }

        case AudioAction::StartRecord: {
            i2s_rx_.Enable();

            bool result =
                rec_ctrl_.Start(fsm_params_.rec_path, fsm_params_.sample_rate_hz, fsm_params_.bits, fsm_params_.ch);

            if (!result) {
                i2s_rx_.Disable();
                PublishNotification(AudioNotification::Type::kError, static_cast<int32_t>(Error::kRecordStartFailed));
                PushFsmEvent(AudioFsmEvent::RecEnded);
            }
            return;
        }

        case AudioAction::StopRecord: {
            rec_ctrl_.RequestStop();
            return;
        }

        default: {
            return;
        }
    }
}

/**
 * @brief 通知キューへイベントを発行する
 *
 * @param[in] type 通知種別
 * @param[in] err エラーコード
 */
void AudioEngine::PublishNotification(AudioNotification::Type type, int32_t err) { notification_queue_.Enqueue(type, err); }

/**
 * @brief 通知キューから1件取り出す
 *
 * @param[out] noti 取り出し先
 * @retval true  取得成功
 * @retval false イベントなし
 */
bool AudioEngine::GetNextNotification(AudioNotification *noti) { return notification_queue_.Dequeue(noti); }

/**
 * @brief FSMイベントを1件保留する
 *
 * @param[in] event 保留するイベント
 */
void AudioEngine::PushFsmEvent(AudioFsmEvent event)
{
    event_pending_ = true;
    event_         = event;
}

/**
 * @brief 再生系がアクティブか判定する
 *
 * @retval true  Playing/Paused
 * @retval false それ以外
 */
bool AudioEngine::IsPlaybackActive() const
{
    const auto state = fsm_.GetState();
    return (state == AudioState::Playing) || (state == AudioState::Paused);
}

/**
 * @brief 録音系がアクティブか判定する
 *
 * @retval true  Recording
 * @retval false それ以外
 */
bool AudioEngine::IsRecordActive() const
{
    const auto state = fsm_.GetState();
    return state == AudioState::Recording;
}

}  // namespace app
}  // namespace core0
