/**
 * @file audio_ipc_controller.cpp
 * @brief audio_ipc_controllerの実装
 */

// 自ヘッダー
#include "audio_ipc_controller.h"

// 標準ライブラリ
#include <cstdio>

// プロジェクトライブラリ
#include "acu_core.h"
#include "logger_core.h"
#include "mapper/agc_config_mapper.h"
#include "utility.h"
#include "wav_reader.h"

namespace core0 {
namespace app {

/**
 * @brief AppServerの各コマンドハンドラを登録する
 */
void AudioIpcController::SetHandlers()
{
    // 再生制御
    server_.on_play   = [&](const core::ipc::PlayPayload &p) -> int32_t { return OnPlay(p); };
    server_.on_pause  = [&]() -> int32_t { return OnPause(); };
    server_.on_resume = [&]() -> int32_t { return OnResume(); };

    // 録音制御
    server_.on_rec_start = [&](const core::ipc::RecStartPayload &p) -> int32_t { return OnRecStart(p); };
    server_.on_rec_stop  = [&]() -> int32_t { return OnRecStop(); };

    // ディレクトリ列挙
    server_.on_list_dir = [&](const core::ipc::ListDirPayload &p, uint32_t seq) -> int32_t {
        return OnListDir(p, seq);
    };

    // AGC設定更新
    server_.on_set_agc = [&](const core::ipc::SetAgcPayload &p) -> int32_t {
        auto cfg = ToAgcConfig(p);

        platform::Agc::GetInstance().ApplyConfig(cfg);

        LOGI("SetAgc applied: en=%u dist=%d min=%d max=%d k=%d", (unsigned)p.enable, (int)p.dist_mm,
             (int)p.min_gain_x100, (int)p.max_gain_x100, (int)p.speed_k);

        return 0;
    };

    // ACU設定更新
    server_.on_set_rec_option = [&](const core::ipc::RecOptionPayload &p) -> int32_t {
        platform::Acu::DcCutConfig dc{};
        dc.fs_hz   = 48000.0f;
        dc.fc_hz   = static_cast<float>(p.dc_fc_q16) / 65536.0f;
        dc.dc_pass = p.dc_enable ? 0u : 1u;

        platform::Acu::NoiseGateConfig ng{};
        ng.th_open   = static_cast<float>(p.ng_th_open_q15) / 32768.0f;
        ng.th_close  = static_cast<float>(p.ng_th_close_q15) / 32768.0f;
        ng.attack_s  = static_cast<float>(p.ng_attack_ms) / 1000.0f;
        ng.release_s = static_cast<float>(p.ng_release_ms) / 1000.0f;
        ng.ng_pass   = p.ng_enable ? 0u : 1u;

        auto &acu = platform::Acu::GetInstance();
        acu.SetDcCut(dc);
        acu.SetNoiseGate(ng);

        LOGI("SetRecOption applied: dc_en=%u fc_q16=%ld ng_en=%u open_q15=%ld close_q15=%ld atk=%u rel=%u",
             (unsigned)p.dc_enable, (long)p.dc_fc_q16, (unsigned)p.ng_enable, (long)p.ng_th_open_q15,
             (long)p.ng_th_close_q15, (unsigned)p.ng_attack_ms, (unsigned)p.ng_release_ms);

        return 0;
    };
}

/**
 * @brief IPC処理ループを実行する
 */
void AudioIpcController::Run()
{
    while (1) {
        // 受信コマンドを処理する
        server_.Task(8);

        // AudioEngineの周期処理を進める
        sys_.Task();

        // 状態通知をIPCへ返す
        NotifyStatus();
    }
}

/**
 * @brief 再生開始要求を処理する
 *
 * @param[in] p 再生要求ペイロード
 * @retval 0 受理
 */
int32_t AudioIpcController::OnPlay(const core::ipc::PlayPayload &p)
{
    LOGI("Received Play message");
    sys_.RequestPlay(p.filename);
    return 0;
}

/**
 * @brief 一時停止要求を処理する
 *
 * @retval 0 受理
 */
int32_t AudioIpcController::OnPause()
{
    LOGI("Received Pause message");
    sys_.RequestPause();
    return 0;
}

/**
 * @brief 再開要求を処理する
 *
 * @retval 0 受理
 */
int32_t AudioIpcController::OnResume()
{
    LOGI("Received Resume message");
    sys_.RequestResume();
    return 0;
}

/**
 * @brief 録音開始要求を処理する
 *
 * @param[in] p 録音開始要求ペイロード
 * @retval 0 受理
 * @retval -1 要求失敗
 */
int32_t AudioIpcController::OnRecStart(const core::ipc::RecStartPayload &p)
{
    // ファイル名は連番で自動採番する
    char auto_name[sizeof(p.filename)]{};
    std::snprintf(auto_name, sizeof(auto_name), "/record_%05u.wav", (unsigned)next_rec_index_);

    LOGI("Received Recording start message : filename=%s", auto_name);

    if (!sys_.RequestRecord(auto_name, p.sample_rate_hz, p.bits, p.ch)) {
        LOGE("RequestRecord failed: filename=%s", auto_name);
        return -1;
    }

    // 次回採番を更新する
    if (next_rec_index_ < kMaxRecIndex) {
        ++next_rec_index_;
    } else {
        next_rec_index_ = 1;
    }

    return 0;
}

/**
 * @brief 録音停止要求を処理する
 *
 * @retval 0 受理
 */
int32_t AudioIpcController::OnRecStop()
{
    sys_.RequestRecordStop();
    return 0;
}

/**
 * @brief WAV一覧要求を処理する
 *
 * @param[in] p 一覧要求ペイロード
 * @param[in] seq 応答シーケンス番号
 * @retval 0 一覧送信成功
 * @retval -1 一覧取得失敗
 */
int32_t AudioIpcController::OnListDir(const core::ipc::ListDirPayload &p, uint32_t seq)
{
    // SDのmount/unmount干渉を避けるため、再生/録音中の一覧取得は拒否する（想定はしていないが一応）
    if (sys_.IsPlaybackActive() || sys_.IsRecordActive()) {
        LOGW("ListDir rejected while audio active");
        return -1;
    }

    // 空文字はルートとして扱う
    const char *path = (p.path[0] != '\0') ? p.path : "/";

    // 1リクエストあたり上限件数まで送信する
    uint16_t   sent = 0;
    const bool ok   = module::WavReader::EnumerateWavFiles(
        path, kMaxDirEntriesPerReq, [&](const module::WavReader::WavFileInfo &info) -> bool {
            server_.SendDirEntry(seq, sent, 0, info.path, info.size, 0);
            ++sent;
            return true;
        });

    if (!ok) {
        LOGE("ListDir: EnumerateWavFiles failed path=%s", path);
        return -1;
    }

    LOGI("ListDir: path=%s sent=%u", path, (unsigned)sent);
    return 0;
}

/**
 * @brief AudioEngineからの通知をIPCステータス送信へ変換する
 */
void AudioIpcController::NotifyStatus()
{
    // 通知を取り出して、必要なステータスを即時送信する
    AudioNotification noti;
    while (sys_.GetNextNotification(&noti)) {
        switch (noti.type) {
            case AudioNotification::Type::kPlaybackStarted:
            case AudioNotification::Type::kPlaybackPaused:
            case AudioNotification::Type::kPlaybackResumed:
            case AudioNotification::Type::kPlaybackStopped:
                SendPlaybackStatus();
                break;

            case AudioNotification::Type::kRecordStarted:
                SendRecordStatus();
                break;
            case AudioNotification::Type::kRecordStopped:
                SendRecordStatus();
                break;

            case AudioNotification::Type::kError:
                // 必要ならエラー通知
                break;

            default:
                break;
        }
    }

    // 即時通知とは別に、定期ステータスも送る
    SendStatusRegularly();
}

/**
 * @brief 再生ステータスを送信する
 */
void AudioIpcController::SendPlaybackStatus()
{
    core::ipc::PlaybackStatusPayload st{};
    if (sys_.GetStatus(&st)) {
        server_.SendPlaybackStatus(st);
    }
}

/**
 * @brief 録音ステータスを送信する
 */
void AudioIpcController::SendRecordStatus()
{
    core::ipc::RecordStatusPayload st{};
    if (sys_.GetStatus(&st)) {
        server_.SendRecordStatus(st);
    }
}

/**
 * @brief アクティブ中の再生/録音ステータスを周期送信する
 */
void AudioIpcController::SendStatusRegularly()
{
    if (sys_.IsPlaybackActive()) {
        const uint32_t now = GetTimeMs();
        if ((now - last_pb_status_ms_) < kStatusPeriodMs) {
            // 周期未満でも送信する現行挙動を維持する
        }
        last_pb_status_ms_ = now;

        SendPlaybackStatus();
    }

    if (sys_.IsRecordActive()) {
        const uint32_t now = GetTimeMs();
        if ((now - last_rec_status_ms_) < kStatusPeriodMs) {
            // 周期未満でも送信する現行挙動を維持する
        }
        last_rec_status_ms_ = now;

        SendRecordStatus();
    }
}

}  // namespace app
}  // namespace core0
