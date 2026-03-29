/**
 * @file playback_controller.cpp
 * @brief playback_controllerの実装
 */

// 自ヘッダー
#include "playback_controller.h"

// 標準ライブラリ
#include <algorithm>
#include <cstring>

// Vitisライブラリ
#include "xparameters.h"

// プロジェクトライブラリ
#include "audio_format.h"
#include "logger_core.h"

namespace core0 {
namespace app {

/**
 * @brief 再生制御に使用する依存オブジェクトをバインドする
 *
 * @param tx AudioFormatterTxインスタンス
 * @param tx_pool 再生用BPPプール
 */
void PlaybackController::Bind(module::AudioFormatterTx &tx, module::AudioBppPool &tx_pool)
{
    tx_      = &tx;
    tx_pool_ = &tx_pool;
}

/**
 * @brief 再生制御の内部状態を初期状態へ戻す
 */
void PlaybackController::Reset()
{
    state_ = State::kIdle;

    pause_req_  = false;
    resume_req_ = false;

    tx_ring_base_ = 0;

    total_bytes_     = 0;
    submitted_bytes_ = 0;
    finishing_       = false;
    needs_final_ioc_ = false;
    fin_tick_        = 0;
    bytes_per_sec_   = 0;

    feeder_.Deinit();

    event_pending_ = false;
    event_         = {};
}

/**
 * @brief 再生開始処理を行う
 *
 * @param wav_path SD再生時のWAVパス（DDR再生時は未使用）
 * @retval true  再生開始に成功
 * @retval false パラメータ不正または初期化失敗
 */
bool PlaybackController::Start(const char *wav_path)
{
    LOG_SCOPE();

    // エラー状態で取り残された場合は、開始要求時に自己復帰してから進む
    if (state_ == State::kError) {
        Reset();
    }

    const bool use_sd_source = (source_mode_ == SourceMode::kSd);

    if (!tx_ || !tx_pool_ || (use_sd_source && (!wav_path || wav_path[0] == '\0'))) {
        // エラーは分けた方がいいがめんどくさいので一括で
        LOGE("Invalid parameters : tx=%p, tx_pool=%p, wav_path=%p", tx_, tx_pool_, wav_path);
        Notify(Event::kError, static_cast<int32_t>(Error::kInvalidParam));
        state_ = State::kError;
        return false;
    }
    if (!use_sd_source && !ddr_buf_) {
        LOGE("DdrAudioBuffer is not bound");
        Notify(Event::kError, static_cast<int32_t>(Error::kDdrBufferNotBound));
        state_ = State::kError;
        return false;
    }

    // 保険でガードしておく
    if (IsActive()) {
        return false;
    }

    // 領域0クリア
    const uintptr_t pool_base  = tx_->GetBufBase();
    const uintptr_t pool_bytes = kBytesPerPeriod * kPeriodsPerChunk;
    tx_ring_base_              = pool_base + pool_bytes;
    std::memset(reinterpret_cast<void *>(tx_ring_base_ + 0), 0, kPeriodsPerHalfBytes);
    std::memset(reinterpret_cast<void *>(tx_ring_base_ + kPeriodsPerHalfBytes), 0, kPeriodsPerHalfBytes);

    tx_->SetBufferBase(tx_ring_base_);

    if (use_sd_source) {
        if (!feeder_.Init(wav_path)) {
            LOGE("feeder_.Init failed");
            Reset();

            Notify(Event::kError, static_cast<int32_t>(Error::kFeederInitFailed));
            state_ = State::kError;
            return false;
        }
        total_bytes_   = feeder_.GetTotalBytes();
        bytes_per_sec_ = feeder_.GetBytesPerSec();
    } else {
        if (!ddr_buf_->ResetForPlay()) {
            LOGE("DdrAudioBuffer reset for play failed");
            Reset();
            Notify(Event::kError, static_cast<int32_t>(Error::kInvalidParam));
            state_ = State::kError;
            return false;
        }
        total_bytes_ = ddr_buf_->GetValidBytes();
        if (total_bytes_ == 0u) {
            LOGE("DdrAudioBuffer has no data");
            Reset();
            Notify(Event::kError, static_cast<int32_t>(Error::kDdrNoData));
            state_ = State::kError;
            return false;
        }
        bytes_per_sec_ = kBytesPerSecond;
    }

    if (bytes_per_sec_ == 0) {
        LOGE("bytes_per_sec is zero");
        Reset();
        Notify(Event::kError, static_cast<int32_t>(Error::kInvalidBytesPerSec));
        state_ = State::kError;
        return false;
    }

    LOGI("PB Start : src=%s path=%s total_bytes=%u bytes_per_sec=%u",
         use_sd_source ? "SD" : "DDR", use_sd_source ? wav_path : "-", (unsigned)total_bytes_, (unsigned)bytes_per_sec_);

    // 先行充填
    FillSide(module::AudioFormatterTx::BufferSide::Ping);
    FillSide(module::AudioFormatterTx::BufferSide::Pong);

    // AF（MM2S）制御開始
    const uint32_t AF_BPP_PLAY = kAESBlockBytes * kPeriodsPerHalf;

    submitted_bytes_ = 0;
    finishing_       = false;
    needs_final_ioc_ = false;
    fin_tick_        = 0;
    pause_req_       = false;
    resume_req_      = false;

    tx_->ClearIocCount();
    tx_->SetChunk(kPeriodsPerChunk, AF_BPP_PLAY);
    tx_->EnableIoc();
    tx_->Start();

    SetNextState(State::kRunning);
    Notify(Event::kStarted, static_cast<int32_t>(Error::kNone));

    return true;
}

/**
 * @brief 現在状態に応じた再生処理を1回分進める
 */
void PlaybackController::Process()
{
    switch (state_) {
        case State::kIdle:
            return;
        case State::kRunning:
            ExecuteRunning();
            return;
        case State::kPaused:
            ExecutePaused();
            return;
        case State::kFinishing:
            ExecuteFinishing();
            return;
        case State::kError:
        default:
            return;
    }
}

/**
 * @brief 保留中イベントを1件取り出す
 *
 * @param event 取り出し先
 * @retval true  取得成功
 * @retval false イベントなし、またはeventがnull
 */
bool PlaybackController::PopEvent(EventInfo *event)
{
    if (!event) {
        return false;
    }

    if (!event_pending_) {
        return false;
    }

    *event         = event_;
    event_pending_ = false;
    event_         = {};

    return true;
}

/**
 * @brief 再生ステータスを取得する
 *
 * @param status 取得先ステータス
 * @retval true  取得成功
 * @retval false 引数不正または内部状態不整合
 */
bool PlaybackController::GetStatus(core::ipc::PlaybackStatusPayload *status) const
{
    if (!status) {
        return false;
    }

    core::ipc::PlaybackStatusPayload payload{};
    payload.position_ms = 0;
    payload.duration_ms = 0;
    payload.remain_ms   = 0;
    payload.state       = 0;

    // Idleなら0で返す
    if (state_ == State::kIdle) {
        *status = payload;
        return true;
    }

    // 再生中なのにbytes_per_secが無いのは異常
    if (bytes_per_sec_ == 0) {
        *status = payload;
        return false;
    }

    // duration_ms = total_bytes / bytes_per_sec * 1000
    const uint64_t dur_ms = (uint64_t)total_bytes_ * 1000ull / (uint64_t)bytes_per_sec_;

    // position_ms = min(played, total) / bytes_per_sec * 1000
    const uint32_t played_clamped = (submitted_bytes_ > total_bytes_) ? total_bytes_ : submitted_bytes_;
    const uint64_t pos_ms         = (uint64_t)played_clamped * 1000ull / (uint64_t)bytes_per_sec_;

    payload.duration_ms = (dur_ms > 0xFFFFFFFFull) ? 0xFFFFFFFFu : (uint32_t)dur_ms;
    payload.position_ms = (pos_ms > 0xFFFFFFFFull) ? 0xFFFFFFFFu : (uint32_t)pos_ms;

    payload.remain_ms = (payload.duration_ms >= payload.position_ms) ? (payload.duration_ms - payload.position_ms) : 0;

    switch (state_) {
        case State::kRunning:
        case State::kFinishing:
            payload.state = 1;
            break;
        case State::kPaused:
            payload.state = 2;
            break;
        case State::kError:  // エラーは作ったけど考えていないので終了と同じで
        default:
            payload.state = 0;
            break;
    }

    *status = payload;

    return true;
}

/**
 * @brief 録再実行中の処理を行う
 */
void PlaybackController::ExecuteRunning()
{
    // Pause
    if (pause_req_) {
        pause_req_ = false;
        tx_->Stop();
        SetNextState(State::kPaused);
        Notify(Event::kPaused, static_cast<int32_t>(Error::kNone));
        return;
    }

    uint16_t processed_side_count = 0;

    module::AudioFormatterTx::BufferSide ready_buffer_side = module::AudioFormatterTx::BufferSide::None;

    // 1回でbuffer-side通知を捌きすぎるとUI/IPCが詰まるかも。
    // 逆に少なすぎると再生/録音が追いつかない。
    // 現状の負荷で安全だった上限。
    static constexpr uint16_t kMaxBufferSidesPerTick = 4;

    while (processed_side_count < kMaxBufferSidesPerTick && tx_->ConsumeReadyBufferSide(&ready_buffer_side)) {
        FillSide(ready_buffer_side);

        ++processed_side_count;

        const uint32_t remain = (total_bytes_ > submitted_bytes_) ? (total_bytes_ - submitted_bytes_) : 0;
        const uint32_t step   = std::min<uint32_t>(kPeriodsPerHalfBytes, remain);
        submitted_bytes_ += step;

        if (!finishing_ && (submitted_bytes_ >= total_bytes_)) {
            if (source_mode_ == SourceMode::kSd) {
                // 末尾到達後は追加投入データは不要。
                // 先読みで溜まったBPPが残ると終了判定が詰まるため、ここで破棄する。
                module::AudioBppPool::Bpp stale{};
                while (tx_pool_->AcquireForTx(&stale)) {
                    tx_pool_->Recycle(stale);
                }
            }

            finishing_       = true;
            needs_final_ioc_ = true;
            SetNextState(State::kFinishing);
            return;
        }
    }

    if (source_mode_ == SourceMode::kSd) {
        // すでにWAVを最後まで読み切っているなら何もしない
        if (feeder_.GetEof()) {
            return;
        }

        // プール残量がここ以下になったら「緊急状態」
        static constexpr uint32_t kLowWatermark = kPeriodsPerHalf;
        // 緊急時に一気に補充してよい最大量
        static constexpr uint32_t kEmergencyBudget = kPeriodsPerHalf;
        // 平常時の補充上限（実測チューニング）
        static constexpr uint32_t kNormalBudget = 12;

        const bool     emergency = (tx_pool_->GetBufferedCount() <= kLowWatermark);
        const uint32_t budget    = emergency ? kEmergencyBudget : kNormalBudget;

        uint32_t done = 0;
        while (tx_pool_->GetBufferedCount() < kPeriodsPerChunk && done < budget) {
            if (!feeder_.ProvideBpp(tx_pool_)) {
                LOGW("PB refill stopped : provide failed (level=%u emergency=%u budget=%u eof=%u)",
                     (unsigned)tx_pool_->GetBufferedCount(), (unsigned)emergency, (unsigned)budget,
                     (unsigned)feeder_.GetEof());
                break;
            }
            ++done;
        }
    }

    if (tx_->GetIocCount() >= kPeriodsPerChunk) {
        tx_->ClearIocCount();
    }
}

/**
 * @brief イベントキューへ通知を1件セットする
 *
 * @param type 通知イベント種別
 * @param err エラーコード
 */
void PlaybackController::Notify(Event type, int32_t err)
{
    event_.type    = type;
    event_.err     = err;
    event_pending_ = true;
}

/**
 * @brief 一時停止中の処理を行う
 */
void PlaybackController::ExecutePaused()
{
    if (resume_req_) {
        resume_req_ = false;

        tx_->ClearIocCount();
        tx_->Reset();
        tx_->EnableIoc();
        tx_->Start();

        SetNextState(State::kRunning);
        Notify(Event::kResumed, static_cast<int32_t>(Error::kNone));

        return;
    }
}

/**
 * @brief 終了待機中の処理を行う
 */
void PlaybackController::ExecuteFinishing()
{
    ++fin_tick_;

    module::AudioFormatterTx::BufferSide ready_buffer_side = module::AudioFormatterTx::BufferSide::None;
    while (tx_->ConsumeReadyBufferSide(&ready_buffer_side)) {
        FillSide(ready_buffer_side);
    }

    if (tx_->GetIocCount() >= kPeriodsPerChunk) {
        tx_->ClearIocCount();

        if (needs_final_ioc_) {
            LOGI("PB finishing : got first final IOC, waiting one more");
            needs_final_ioc_ = false;
        } else {
            LOGI("PB finishing : stop by IOC path");
            Finish(Event::kStopped, static_cast<int32_t>(Error::kNone));
            return;
        }
    }

    // フェイルセーフ
    // DMA/buffer-side通知が来ない場合でも、TXプールが空なら供給側データは尽きている
    // feeder_.GetEof() は「ReadSomeをさらに呼んだ」時にしか立たないため、
    // 末尾ぴったりで終了したファイルではfalseのまま止まるケースがある
    if (source_mode_ == SourceMode::kSd && finishing_ && (tx_pool_->GetBufferedCount() == 0)) {
        LOGI("PB finishing : stop by pool-empty path (tick=%u, ioc=%u)", (unsigned)fin_tick_,
             (unsigned)tx_->GetIocCount());
        Finish(Event::kStopped, static_cast<int32_t>(Error::kNone));
        return;
    }
}

/**
 * @brief 指定half面（Ping/Pong）へデータを充填する
 *
 * @param which 充填対象のhalf面
 */
void PlaybackController::FillSide(module::AudioFormatterTx::BufferSide which)
{
    const uintptr_t dst_base =
        (which == module::AudioFormatterTx::BufferSide::Ping) ? tx_ring_base_ : (tx_ring_base_ + kPeriodsPerHalfBytes);
    size_t filled = 0;

    if (source_mode_ == SourceMode::kSd) {
        while (filled < kPeriodsPerHalfBytes) {
            module::AudioBppPool::Bpp bpp{};
            if (!tx_pool_->AcquireForTx(&bpp)) {
                break;
            }

            const size_t copy = std::min<size_t>(bpp.valid_bytes, kPeriodsPerHalfBytes - filled);
            if (copy > 0) {
                std::memcpy(reinterpret_cast<void *>(dst_base + filled), bpp.addr, copy);
                filled += copy;
            }
            tx_pool_->Recycle(bpp);
        }
    } else if (ddr_buf_) {
        while (filled < kPeriodsPerHalfBytes) {
            const uint32_t req   = static_cast<uint32_t>(kPeriodsPerHalfBytes - filled);
            const uint32_t bytes = ddr_buf_->ReadSome(reinterpret_cast<void *>(dst_base + filled), req);
            if (bytes == 0u) {
                break;
            }
            filled += bytes;
        }
    }

    if (filled < kPeriodsPerHalfBytes) {
        std::memset(reinterpret_cast<void *>(dst_base + filled), 0, kPeriodsPerHalfBytes - filled);
    }
}

/**
 * @brief 再生終了共通処理
 *
 * @param event 通知するイベント種別
 * @param err   エラーコード
 */
void PlaybackController::Finish(Event event, int32_t err)
{
    if (tx_) {
        tx_->Stop();
    }

    if (source_mode_ == SourceMode::kSd) {
        feeder_.Deinit();
    }

    finishing_       = false;
    needs_final_ioc_ = false;
    pause_req_       = false;
    resume_req_      = false;
    submitted_bytes_ = 0;
    total_bytes_     = 0;
    fin_tick_        = 0;

    SetNextState(State::kIdle);
    Notify(event, err);
}

}  // namespace app
}  // namespace core0
