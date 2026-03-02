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
#include "i2s_rx_core.h"
#include "i2s_tx_core.h"
#include "logger_core.h"

namespace core0 {
namespace app {

/**
 * @brief 初期化処理
 *
 * @retval true  初期化成功
 * @retval false 初期化失敗
 */
bool AudioEngine::Init()
{
    LOG_SCOPE();

    notification_queue_.Reset();
    fsm_ctx_.Reset();

    if (!pipeline_.Init(ddr_base_addr_, platform::Acu::GetInstance(), XPAR_XAUDIO_CLEAN_UP_0_BASEADDR, i2s_tx_,
                        i2s_rx_)) {
        LOGE("pipeline_.Init failed");
        return false;
    }

    auto *tx = pipeline_.tx();
    auto *rx = pipeline_.rx();
    if (!tx || !rx) {
        LOGE("pipeline tx/rx is null: tx=%p rx=%p", tx, rx);
        return false;
    }

    // 送受信用プールを準備する
    if (!tx_pool_.Init(ddr_base_addr_, module::AudioBppPool::kBppSize, module::AudioBppPool::kBppsPerChunk)) {
        LOGE("Failed to allocate memory pool on the sending side");
        return false;
    }
    if (!rx_pool_.Init(ddr_base_addr_, module::AudioBppPool::kBppSize, module::AudioBppPool::kBppsPerChunk)) {
        LOGE("Failed to allocate memory pool on the receiving side");
        return false;
    }

    pb_ctrl_.Bind(*tx, tx_pool_);
    rec_ctrl_.Bind(*rx, rx_pool_);

    return true;
}

/**
 * @brief 終了処理
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
    fsm_ctx_.Reset();
}

/**
 * @brief 再生要求受け付け処理
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

    fsm_ctx_.params.play_path = play_path_;
    fsm_ctx_.SetEvent(AudioFsmEvent::UiPlayPressed);

    return true;
}

/**
 * @brief 録音要求受け付け処理
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

    fsm_ctx_.params.rec_path       = rec_path_;
    fsm_ctx_.params.sample_rate_hz = sample_rate_hz;
    fsm_ctx_.params.bits           = bits;
    fsm_ctx_.params.ch             = ch;

    fsm_ctx_.SetEvent(AudioFsmEvent::UiRecPressed);

    return true;
}

/**
 * @brief 録音停止要求受け付け処理
 *
 * @retval true 常にtrue
 */
bool AudioEngine::RequestRecordStop()
{
    if (fsm_ctx_.fsm.GetState() == AudioState::Recording) {
        fsm_ctx_.SetEvent(AudioFsmEvent::UiRecPressed);
    }

    return true;
}

/**
 * @brief 一時停止要求受け付け処理
 */
void AudioEngine::RequestPause()
{
    if (fsm_ctx_.fsm.GetState() == AudioState::Playing) {
        fsm_ctx_.SetEvent(AudioFsmEvent::UiPlayPressed);
    }
}

/**
 * @brief 再開要求受け付け処理
 */
void AudioEngine::RequestResume()
{
    if (fsm_ctx_.fsm.GetState() == AudioState::Paused) {
        fsm_ctx_.SetEvent(AudioFsmEvent::UiPlayPressed);
    }
}

/**
 * @brief 周期処理
 */
void AudioEngine::Task()
{
    // FSMイベントがあれば dispatch → action実行
    RunFsm();

    // 状態に応じて駆動
    switch (fsm_ctx_.fsm.GetState()) {
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
 * @brief 再生系の周期処理を実行
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
                notification_queue_.Enqueue(AudioNotification::Type::kPlaybackStarted,
                                            static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kPaused:
                notification_queue_.Enqueue(AudioNotification::Type::kPlaybackPaused,
                                            static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kResumed:
                notification_queue_.Enqueue(AudioNotification::Type::kPlaybackResumed,
                                            static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kStopped:
                notification_queue_.Enqueue(AudioNotification::Type::kPlaybackStopped,
                                            static_cast<int32_t>(Error::kNone));
                i2s_tx_.Disable();
                fsm_ctx_.SetEvent(AudioFsmEvent::PbEnded);
                break;
            case PlaybackController::Event::kError:
                notification_queue_.Enqueue(AudioNotification::Type::kError, pev.err);
                i2s_tx_.Disable();
                fsm_ctx_.SetEvent(AudioFsmEvent::PbEnded);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief 録音系の周期処理を実行
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
                notification_queue_.Enqueue(AudioNotification::Type::kRecordStarted,
                                            static_cast<int32_t>(Error::kNone));
                break;
            case RecordController::Event::kStopped:
                notification_queue_.Enqueue(AudioNotification::Type::kRecordStopped,
                                            static_cast<int32_t>(Error::kNone));
                i2s_rx_.Disable();
                fsm_ctx_.SetEvent(AudioFsmEvent::RecEnded);
                break;
            case RecordController::Event::kError:
                notification_queue_.Enqueue(AudioNotification::Type::kError, rev.err);
                fsm_ctx_.SetEvent(AudioFsmEvent::RecEnded);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief FSMイベントを処理しアクションを実行
 */
void AudioEngine::RunFsm()
{
    AudioTransition transition{};

    if (fsm_ctx_.Dispatch(&transition)) {
        // 1イベントに対して最大2アクションを順に実行する
        ExecuteFsmAction(transition.action1);
        ExecuteFsmAction(transition.action2);
    }
}

/**
 * @brief FSMアクションを実行
 *
 * @param[in] action 実行するアクション
 */
void AudioEngine::ExecuteFsmAction(AudioAction action)
{
    switch (action) {
        case AudioAction::None: {
            return;
        }

        case AudioAction::StartPlayback: {
            i2s_tx_.Enable();

            bool result = pb_ctrl_.Start(fsm_ctx_.params.play_path);

            if (!result) {
                i2s_tx_.Disable();
                notification_queue_.Enqueue(AudioNotification::Type::kError,
                                            static_cast<int32_t>(Error::kPlaybackStartFailed));
                fsm_ctx_.SetEvent(AudioFsmEvent::PbEnded);
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

            bool result = rec_ctrl_.Start(fsm_ctx_.params.rec_path, fsm_ctx_.params.sample_rate_hz,
                                          fsm_ctx_.params.bits, fsm_ctx_.params.ch);

            if (!result) {
                i2s_rx_.Disable();
                notification_queue_.Enqueue(AudioNotification::Type::kError,
                                            static_cast<int32_t>(Error::kRecordStartFailed));
                fsm_ctx_.SetEvent(AudioFsmEvent::RecEnded);
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
 * @brief 再生系がアクティブか判定
 *
 * @retval true  Playing/Paused
 * @retval false それ以外
 */
bool AudioEngine::IsPlaybackActive() const
{
    const auto state = fsm_ctx_.fsm.GetState();
    return (state == AudioState::Playing) || (state == AudioState::Paused);
}

/**
 * @brief 録音系がアクティブか判定
 *
 * @retval true  Recording
 * @retval false それ以外
 */
bool AudioEngine::IsRecordActive() const
{
    const auto state = fsm_ctx_.fsm.GetState();
    return state == AudioState::Recording;
}

}  // namespace app
}  // namespace core0
