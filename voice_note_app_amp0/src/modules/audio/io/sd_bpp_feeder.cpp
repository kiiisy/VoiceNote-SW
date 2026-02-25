/**
 * @file sd_bpp_feeder.cpp
 * @brief SdBppFeederの実装
 */

// 自ヘッダー
#include "sd_bpp_feeder.h"

// 標準ライブラリ
#include <cstring>
#include <new>

// プロジェクトライブラリ
#include "audio_bpp_pool.h"
#include "logger_core.h"

namespace core0 {
namespace module {

SdBppFeeder::~SdBppFeeder() { Deinit(); }

/**
 * @brief SDをマウントし、WAVをオープンしてヘッダ確認まで行う
 *
 * @param[in] wav_path SD上のWAVファイルパス
 * @retval true  成功
 * @retval false 失敗
 */
bool SdBppFeeder::Init(const char *wav_path)
{
    LOG_SCOPE();

    Deinit();

    if (!wav_path || wav_path[0] == '\0') {
        LOGE("Invalid path");
        return false;
    }

    if (!fs_.Mount()) {
        LOGE("Mount failed");
        Deinit();
        return false;
    }

    if (!file_.OpenRead(wav_path)) {
        LOGE("Open failed (%s)", wav_path);
        Deinit();
        return false;
    }

    if (!reader_.Open(&file_)) {
        LOGE("Wav header parse failed");
        Deinit();
        return false;
    }

    total_bytes_ = reader_.GetDataBytesTotal();
    eof_         = false;
    ready_       = true;

    return true;
}

/**
 * @brief 確保したリソースを解放する
 */
void SdBppFeeder::Deinit()
{
    ready_       = false;
    eof_         = false;
    total_bytes_ = 0;

    // 閉じる順固定（wav -> file -> fs）
    reader_.Close();  // 借用解除
    file_.Close();    // ファイルハンドルを閉じる
    fs_.Unmount();    // mount済みなら外す
}

/**
 * @brief プールに1BPP分のPCMを供給する
 *
 * @param[in,out] pool 供給先プール
 * @retval true  供給成功（BPP を 1 個 Commit 済み）
 * @retval false 供給失敗（未初期化、空き無しなど）
 *
 * @note 制約: この関数内にエラーログ以外仕込むの禁止
 *        - 低レイテンシ/高頻度呼び出し前提のため、重い処理を入れない意図。
 */
bool SdBppFeeder::ProvideBpp(AudioBppPool *pool)
{
    if (!pool || !ready_) {
        LOGE("Not ready");
        return false;
    }

    AudioBppPool::Bpp bpp;
    if (!pool->Alloc(&bpp)) {
        LOGE("No space in the memory pool");
        return false;
    }

    const int32_t BPP  = AudioBppPool::kBppSize;
    uint8_t      *p    = static_cast<uint8_t *>(bpp.addr);
    int32_t       filled = 0;

    while (filled < BPP) {
        const int32_t need = BPP - filled;

        int32_t bytes_read = reader_.ReadSome(p + filled, static_cast<uint32_t>(need));

        // 読み込み失敗 → 残りを無音
        if (bytes_read < 0) {
            LOGE("ProvideBpp: ReadSome failed (filled=%d need=%d remain=%u)", filled, need,
                 (unsigned)reader_.GetDataBytesRemaining());
            std::memset(p + filled, 0, need);
            filled = BPP;
            eof_   = true;
            break;
        }

        // EOF → 残りを無音
        if (bytes_read == 0) {
            LOGI("ProvideBpp: EOF (filled=%d need=%d remain=%u)", filled, need, (unsigned)reader_.GetDataBytesRemaining());
            std::memset(p + filled, 0, need);
            filled = BPP;
            eof_   = true;
            break;
        }

        filled += bytes_read;
    }

    pool->Commit(bpp, BPP);

    return true;
}

/**
 * @brief 1秒あたりのバイト数を取得する
 *
 * WAVファイルのサンプルレート、チャネル数、ビット深度から算出される
 * バイト数を返す。
 *
 * @return 1秒あたりのバイト数。未初期化の場合は0を返す。
 */
uint32_t SdBppFeeder::GetBytesPerSec() const
{
    // 未初期化なら0を返す
    if (!ready_) {
        return 0;
    }

    const uint32_t sample_rate = reader_.GetSampleRate();
    const uint32_t ch          = reader_.GetChannels();
    const uint32_t bps         = reader_.GetBitsPerSample();

    return sample_rate * ch * (bps / 8u);
}

}  // namespace module
}  // namespace core0
