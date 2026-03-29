/**
 * @file record_controller.cpp
 * @brief record_controllerの実装
 */

// 自ヘッダー
#include "record_controller.h"

// 標準ライブラリ
#include <algorithm>
#include <cstring>

// Vitisライブラリ
#include "xparameters.h"

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace app {

/**
 * @brief 録音制御に使用する依存オブジェクトをバインドする
 *
 * @param rx AudioFormatterRxインスタンス
 * @param rx_pool 録音用BPPプール
 */
void RecordController::Bind(module::AudioFormatterRx &rx, module::AudioBppPool &rx_pool)
{
    rx_      = &rx;
    rx_pool_ = &rx_pool;
}

/**
 * @brief 録音制御の内部状態を初期状態へ戻す
 */
void RecordController::Reset()
{
    state_ = State::kIdle;

    stop_req_ = false;

    rx_ring_base_  = 0;
    written_bytes_ = 0;
    target_bytes_  = 0;

    recorder_.Deinit();

    rx_drop_count_     = 0;
    drop_pending_      = false;
    ddr_write_error_   = false;
    no_progress_ticks_ = 0;

    event_pending_ = false;
    event_         = {};
}

/**
 * @brief 録音開始処理を行う
 *
 * @param wav_path 出力するWAVファイルパス
 * @param sample_rate_hz サンプルレート[Hz]
 * @param bits 量子化ビット数
 * @param ch チャンネル数
 * @retval true 録音開始に成功
 * @retval false パラメータ不正または初期化失敗
 */
bool RecordController::Start(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t ch)
{
    LOG_SCOPE();

    // エラー状態で取り残された場合は、開始要求時に自己復帰してから進む
    if (state_ == State::kError) {
        Reset();
    }

    const bool use_sd = (output_mode_ == OutputMode::kSd);

    // 最低限の入力チェック
    if (!rx_ || !rx_pool_ || sample_rate_hz == 0 || ch == 0 || bits == 0 || (bits % 8u) != 0u ||
        (use_sd && (!wav_path || wav_path[0] == '\0'))) {
        // エラーは分けた方がいいがめんどくさいので一括で
        LOGE("Invalid parameters: rx=%p, rx_pool=%p, wav_path=%p, sr=%u, bits=%u, ch=%u", rx_, rx_pool_, wav_path,
             (unsigned)sample_rate_hz, (unsigned)bits, (unsigned)ch);
        Notify(Event::kError, static_cast<int32_t>(Error::kInvalidParam));
        state_ = State::kError;
        return false;
    }
    if (!use_sd && !ddr_buf_) {
        LOGE("DdrAudioBuffer is not bound");
        Notify(Event::kError, static_cast<int32_t>(Error::kDdrBufferNotBound));
        state_ = State::kError;
        return false;
    }

    // 保険でガードしておく
    if (IsActive()) {
        RequestStop();
        return false;
    }

    rx_drop_count_     = 0;
    drop_pending_      = false;
    ddr_write_error_   = false;
    no_progress_ticks_ = 0;

    // 領域0クリア
    const uintptr_t pool_base  = rx_->GetBufBase();
    const uintptr_t pool_bytes = kBytesPerPeriod * kPeriodsPerChunk;
    rx_ring_base_              = pool_base + pool_bytes;
    std::memset(reinterpret_cast<void *>(rx_ring_base_ + 0), 0, kPeriodsPerHalfBytes);
    std::memset(reinterpret_cast<void *>(rx_ring_base_ + kPeriodsPerHalfBytes), 0, kPeriodsPerHalfBytes);

    rx_->SetBufferBase(rx_ring_base_);

    if (use_sd) {
        if (!recorder_.Init(wav_path, sample_rate_hz, bits, ch)) {
            LOGE("recorder_.Init failed");
            Reset();

            Notify(Event::kError, static_cast<int32_t>(Error::kRecorderInitFailed));
            state_ = State::kError;
            return false;
        }
    } else {
        ddr_buf_->ResetForRecord();
    }

    // 容量/時間計算
    bytes_per_sec_ = static_cast<uint32_t>((uint64_t)sample_rate_hz * ch * (bits / 8));
    target_bytes_  = bytes_per_sec_ * kMaxRecordSeconds;
    if (!use_sd) {
        const uint32_t cap = ddr_buf_->GetCapacityBytes();
        if (cap == 0u) {
            LOGE("DdrAudioBuffer capacity is zero");
            Reset();
            Notify(Event::kError, static_cast<int32_t>(Error::kInvalidParam));
            state_ = State::kError;
            return false;
        }
        target_bytes_ = std::min<uint32_t>(target_bytes_, cap);
        LOGI("REC DDR mode: cap=%u target=%u bytes_per_sec=%u", (unsigned)cap, (unsigned)target_bytes_,
             (unsigned)bytes_per_sec_);
    }
    written_bytes_ = 0;

    // AF（S2MM）制御開始
    rx_->ClearIocCount();
    rx_->SetChunk(kPeriodsPerChunk, kBytesPerPeriod);
    rx_->EnableIoc();
    rx_->Start();

    stop_req_ = false;
    SetNextState(State::kRunning);
    Notify(Event::kStarted, static_cast<int32_t>(Error::kNone));

    return true;
}

/**
 * @brief 現在状態に応じた録音処理を1回分進める
 */
void RecordController::Process()
{
    switch (state_) {
        case State::kIdle:
            return;
        case State::kRunning:
            ExecuteRunning();
            return;
        case State::kStopping:
            ExecuteStopping();
            return;
        case State::kError:
        default:
            return;
    }
}

/**
 * @brief 保留中イベントを1件取り出す
 *
 * @param out 取り出し先
 * @retval true イベント取得成功
 * @retval false イベントなし、またはoutがnull
 */
bool RecordController::PopEvent(EventInfo *out)
{
    if (!out) {
        return false;
    }

    if (!event_pending_) {
        return false;
    }

    *out           = event_;
    event_pending_ = false;
    event_         = {};

    return true;
}

/**
 * @brief 録音ステータスを取得する
 *
 * @param out 取得先ステータス
 * @retval true 取得成功
 * @retval false outがnull、または内部状態不整合
 */
bool RecordController::GetStatus(core::ipc::RecordStatusPayload *out) const
{
    if (!out) {
        return false;
    }

    core::ipc::RecordStatusPayload st{};
    st.state = 0;

    if (state_ == State::kIdle) {
        st.target_ms = kMaxRecordSeconds * 1000;
        *out         = st;
        return true;
    }

    if (bytes_per_sec_ == 0) {
        return false;
    }

    const uint64_t captured_ms = (uint64_t)written_bytes_ * 1000ull / bytes_per_sec_;
    const uint64_t target_ms   = (uint64_t)target_bytes_ * 1000ull / bytes_per_sec_;

    st.captured_ms = (captured_ms > 0xFFFFFFFFull) ? 0xFFFFFFFFu : (uint32_t)captured_ms;
    st.target_ms   = (target_ms > 0xFFFFFFFFull) ? 0xFFFFFFFFu : (uint32_t)target_ms;
    st.remain_ms   = (st.target_ms >= st.captured_ms) ? (st.target_ms - st.captured_ms) : 0;
    st.state       = (state_ == State::kRunning || state_ == State::kStopping) ? 1 : 0;

    *out = st;

    return true;
}

/**
 * @brief イベントキューへ通知を1件セットする
 *
 * @param type 通知イベント種別
 * @param err エラーコード
 */
void RecordController::Notify(Event type, int32_t err)
{
    event_.type    = type;
    event_.err     = err;
    event_pending_ = true;
}

/**
 * @brief 録音実行中の処理を行う
 */
void RecordController::ExecuteRunning()
{
    if (stop_req_) {
        LOGD("stop_req_ is true");
        SetNextState(State::kStopping);
        return;
    }

    uint16_t processed_side_count = 0;

    module::AudioFormatterRx::BufferSide ready_buffer_side = module::AudioFormatterRx::BufferSide::None;

    // 1回でbuffer-side通知を捌きすぎるとUI/IPCが詰まるかも
    // 逆に少なすぎると再生/録音が追いつかない
    // 現状の負荷で安全だった上限
    static constexpr uint16_t kMaxBufferSidesPerTick = 4;

    while (processed_side_count < kMaxBufferSidesPerTick && rx_->ConsumeReadyBufferSide(&ready_buffer_side)) {
        if (ready_buffer_side == module::AudioFormatterRx::BufferSide::Ping ||
            ready_buffer_side == module::AudioFormatterRx::BufferSide::Pong) {
            CopySideBufferToPool(ready_buffer_side);

            // ここでドロップが起きたら、ここで止める（これ以上進めても欠落は埋められない）
            if (drop_pending_) {
                LOGD("Drop RX data");
                // ログ連打を避けStoppingへ
                Notify(Event::kError, static_cast<int32_t>(Error::kRxDrop));
                SetNextState(State::kStopping);
                return;
            }
        }

        ++processed_side_count;

        uint32_t wrote = 0;

        for (;;) {
            uint32_t wrote_bytes = 0;
            if (output_mode_ == OutputMode::kSd) {
                if (!ConsumeOneBppToSd(&wrote_bytes)) {
                    break;
                }
            } else {
                if (!ConsumeOneBppToDdr(&wrote_bytes)) {
                    if (ddr_write_error_) {
                        Notify(Event::kError, static_cast<int32_t>(Error::kDdrWriteFailed));
                        SetNextState(State::kStopping);
                        return;
                    }
                    break;
                }
            }

            ++wrote;

            written_bytes_ += wrote_bytes;

            if (written_bytes_ >= target_bytes_) {
                LOGD("Target bytes reached: %u/%u", written_bytes_, target_bytes_);
                SetNextState(State::kStopping);
                return;
            }

            if (wrote >= kPeriodsPerChunk) {
                break;
            }
        }

        if (output_mode_ == OutputMode::kDdr) {
            if (wrote == 0u) {
                ++no_progress_ticks_;
                if (no_progress_ticks_ == 32u || (no_progress_ticks_ % 128u) == 0u) {
                    LOGW("REC DDR no progress: ticks=%u written=%u target=%u buffered=%d", (unsigned)no_progress_ticks_,
                         (unsigned)written_bytes_, (unsigned)target_bytes_, (int)rx_pool_->GetBufferedCount());
                }
            } else {
                no_progress_ticks_ = 0;
                if (((written_bytes_ / kBytesPerPeriod) % 64u) == 0u) {
                    LOGD("REC DDR progress: written=%u/%u buffered=%d", (unsigned)written_bytes_,
                         (unsigned)target_bytes_, (int)rx_pool_->GetBufferedCount());
                }
            }
        }
    }

    if (rx_->GetIocCount() >= kPeriodsPerChunk) {
        rx_->ClearIocCount();
    }
}

/**
 * @brief 録音停止処理を行う
 */
void RecordController::ExecuteStopping()
{
    LOGI("ExecuteStopping called: mode=%s written=%u target=%u", (output_mode_ == OutputMode::kDdr) ? "DDR" : "SD",
         (unsigned)written_bytes_, (unsigned)target_bytes_);
    if (rx_) {
        rx_->Stop();
    }

    if (output_mode_ == OutputMode::kSd) {
        recorder_.Deinit();
    } else if (ddr_buf_) {
        ddr_buf_->FinalizeRecord();
    }

    if (rx_drop_count_ != 0) {
        LOGE("RX drop detected : drop_count=%u", (unsigned)rx_drop_count_);
    }

    stop_req_      = false;
    written_bytes_ = 0;
    target_bytes_  = 0;

    drop_pending_      = false;
    ddr_write_error_   = false;
    no_progress_ticks_ = 0;

    SetNextState(State::kIdle);
    Notify(Event::kStopped, static_cast<int32_t>(Error::kNone));
}

/**
 * @brief プールから1BPP取り出してSDへ書き出す
 *
 * @param[out] out_bytes 実際に書き込んだバイト数
 * @retval true  1BPP消費成功
 * @retval false データ未到着、またはSD書き出し失敗
 */
bool RecordController::ConsumeOneBppToSd(uint32_t *out_bytes)
{
    if (!out_bytes || !rx_pool_) {
        return false;
    }

    if (!recorder_.ConsumeOneBpp(rx_pool_)) {
        return false;
    }

    *out_bytes = kBytesPerPeriod;
    return true;
}

/**
 * @brief プールから1BPP取り出してDDRバッファへ書き出す
 *
 * @param[out] out_bytes 実際に追記したバイト数
 * @retval true  1BPP消費成功
 * @retval false データ未到着、またはDDR追記失敗
 */
bool RecordController::ConsumeOneBppToDdr(uint32_t *out_bytes)
{
    if (!out_bytes || !ddr_buf_ || !rx_pool_) {
        return false;
    }

    module::AudioBppPool::Bpp bpp{};
    if (!rx_pool_->AcquireForRx(&bpp)) {
        return false;
    }

    const uint32_t bytes = (bpp.valid_bytes > 0) ? static_cast<uint32_t>(bpp.valid_bytes) : 0u;
    bool           ok    = true;
    if (bytes > 0u) {
        ok = ddr_buf_->Append(bpp.addr, bytes);
    }

    rx_pool_->Recycle(bpp);

    if (!ok) {
        LOGE("REC DDR append failed: bytes=%u write_pos=%u cap=%u valid=%u", (unsigned)bytes,
             (unsigned)ddr_buf_->GetWritePos(), (unsigned)ddr_buf_->GetCapacityBytes(),
             (unsigned)ddr_buf_->GetValidBytes());
        ddr_write_error_ = true;
        return false;
    }

    *out_bytes = bytes;
    return true;
}

/**
 * @brief sideバッファの内容をBPPプールへコピーする
 *
 * @param which Ping/Pong のどちらをコピーするか
 * @return 生成できたBPP数
 */
uint32_t RecordController::CopySideBufferToPool(module::AudioFormatterRx::BufferSide which)
{
    const uintptr_t src_base =
        (which == module::AudioFormatterRx::BufferSide::Ping) ? rx_ring_base_ : (rx_ring_base_ + kPeriodsPerHalfBytes);

    uint32_t produced = 0;

    for (uint32_t i = 0; i < kPeriodsPerHalf; ++i) {
        const void *exp_src = reinterpret_cast<const void *>(src_base + i * kBytesPerPeriod);

        module::AudioBppPool::Bpp bpp{};
        if (!rx_pool_->Alloc(&bpp)) {
            ++rx_drop_count_;
            drop_pending_ = true;
            break;
        }

        std::memcpy(bpp.addr, exp_src, kBytesPerPeriod);
        rx_pool_->Produce(bpp, kBytesPerPeriod);
        ++produced;
    }

    return produced;
}

}  // namespace app
}  // namespace core0
