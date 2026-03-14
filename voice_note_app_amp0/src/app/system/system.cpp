/**
 * @file system.cpp
 * @brief Systemの実装
 */

// 自ヘッダー
#include "system.h"

// 標準ライブラリ
#include <algorithm>
#include <cmath>

// Vtiisライブラリ
#include "sleep.h"
#include "xil_mmu.h"
#include "xparameters.h"

// プロジェクトライブラリ
#include "i2s_rx_core.h"
#include "i2s_tx_core.h"
#include "logger_core.h"

namespace core0 {
namespace app {
namespace {
int CppcheckTestFunction()
{
    int *p = nullptr;
    return *p;
}
}  // namespace

/**
 * @brief 初期化処理
 *
 * @retval true  初期化成功
 * @retval false 初期化失敗
 */
bool System::Init()
{
    LOG_SCOPE();

    notification_publisher_.Reset();
    fsm_ctx_.Reset();

    if (!pipeline_.Init(ddr_base_addr_, i2s_tx_, i2s_rx_)) {
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
void System::Deinit()
{
    // HW
    pb_ctrl_.Reset();
    rec_ctrl_.Reset();
    pipeline_.Deinit();

    // pools
    tx_pool_.Deinit();
    rx_pool_.Deinit();

    notification_publisher_.Reset();
    fsm_ctx_.Reset();
}

/**
 * @brief 再生要求受け付け処理
 *
 * @param[in] wav_path 再生対象WAVパス
 * @retval true  要求受理
 * @retval false 引数不正
 */
bool System::EnqueuePlay(const char *wav_path)
{
    if (!wav_path) {
        return false;
    }

    fsm_ctx_.params.play_path = wav_path;
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
bool System::EnqueueRecord(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch)
{
    if (!wav_path) {
        return false;
    }

    fsm_ctx_.params.rec_path       = wav_path;
    fsm_ctx_.params.sample_rate_hz = sample_rate_hz;
    fsm_ctx_.params.bits           = bits;
    fsm_ctx_.params.ch             = ch;

    fsm_ctx_.SetEvent(AudioFsmEvent::UiRecPressed);

    return true;
}

/**
 * @brief 録音トグル要求受け付け処理
 */
void System::EnqueueRecToggle() { fsm_ctx_.SetEvent(AudioFsmEvent::UiRecPressed); }

/**
 * @brief 再生トグル要求受け付け処理
 */
void System::EnqueuePlayToggle() { fsm_ctx_.SetEvent(AudioFsmEvent::UiPlayPressed); }

/**
 * @brief AppServerの各コマンドハンドラを登録する
 */
void System::SetHandlers()
{
    command_handler_.BindHandlers();
    dsp_config_handler_.BindHandlers();
    file_query_handler_.BindHandlers();
}

/**
 * @brief IPC処理ループを実行する
 */
void System::Run()
{
    while (1) {
        // 受信コマンドを処理する
        server_.Task(8);

        // Systemの周期処理を進める
        Task();

        // 状態通知をIPCへ返す
        status_handler_.NotifyStatus();
    }
}

/**
 * @brief 周期処理
 */
void System::Task()
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
void System::ProcessPlayback()
{
    if (pb_ctrl_.IsActive()) {
        pb_ctrl_.Process();
    }

    PlaybackController::EventInfo pev{};
    while (pb_ctrl_.PopEvent(&pev)) {
        switch (pev.type) {
            case PlaybackController::Event::kStarted:
                notification_publisher_.Publish(AudioNotification::Type::kPlaybackStarted,
                                                static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kPaused:
                notification_publisher_.Publish(AudioNotification::Type::kPlaybackPaused,
                                                static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kResumed:
                notification_publisher_.Publish(AudioNotification::Type::kPlaybackResumed,
                                                static_cast<int32_t>(Error::kNone));
                break;
            case PlaybackController::Event::kStopped:
                notification_publisher_.Publish(AudioNotification::Type::kPlaybackStopped,
                                                static_cast<int32_t>(Error::kNone));
                i2s_tx_.Disable();
                fsm_ctx_.SetEvent(AudioFsmEvent::PbEnded);
                break;
            case PlaybackController::Event::kError:
                notification_publisher_.Publish(AudioNotification::Type::kError, pev.err);
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
void System::ProcessRecord()
{
    if (rec_ctrl_.IsActive()) {
        rec_ctrl_.Process();
    }

    RecordController::EventInfo rev{};
    while (rec_ctrl_.PopEvent(&rev)) {
        switch (rev.type) {
            case RecordController::Event::kStarted:
                notification_publisher_.Publish(AudioNotification::Type::kRecordStarted,
                                                static_cast<int32_t>(Error::kNone));
                break;
            case RecordController::Event::kStopped:
                notification_publisher_.Publish(AudioNotification::Type::kRecordStopped,
                                                static_cast<int32_t>(Error::kNone));
                i2s_rx_.Disable();
                fsm_ctx_.SetEvent(AudioFsmEvent::RecEnded);
                break;
            case RecordController::Event::kError:
                notification_publisher_.Publish(AudioNotification::Type::kError, rev.err);
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
void System::RunFsm()
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
void System::ExecuteFsmAction(AudioAction action)
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
                notification_publisher_.Publish(AudioNotification::Type::kError,
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
                notification_publisher_.Publish(AudioNotification::Type::kError,
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
bool System::IsPlaybackActive() const
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
bool System::IsRecordActive() const
{
    const auto state = fsm_ctx_.fsm.GetState();
    return state == AudioState::Recording;
}

}  // namespace app
}  // namespace core0
