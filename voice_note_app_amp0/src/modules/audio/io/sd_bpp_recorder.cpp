/**
 * @file sd_bpp_recorder.cpp
 * @brief SdBppRecorderの実装
 */

// 自ヘッダー
#include "sd_bpp_recorder.h"

// 標準ライブラリ
#include <cstring>

// プロジェクトライブラリ
#include "audio_bpp_pool.h"
#include "logger_core.h"

namespace core0 {
namespace module {

SdBppRecorder::~SdBppRecorder() { Deinit(); }

/**
 * @brief SDをマウントし、WAVをオープンしてヘッダ書き込みまで行う
 *
 * @param[in] wav_path       SD上のWAVファイルパス
 * @param[in] sample_rate_hz サンプルレート
 * @param[in] bits           量子化ビット数
 * @param[in] channels       チャンネル数
 * @retval true  成功
 * @retval false 失敗
 */
bool SdBppRecorder::Init(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t channels)
{
    LOG_SCOPE();

    Deinit();

    if (!wav_path || wav_path[0] == '\0') {
        LOGE("Invalid path");
        return false;
    }

    if (sample_rate_hz == 0 || channels == 0) {
        LOGE("Invalid fmt (sr=%u ch=%u)", (unsigned)sample_rate_hz, (unsigned)channels);
        return false;
    }

    if (!fs_.Mount()) {
        LOGE("Mount failed");
        Deinit();
        return false;
    }

    if (!file_.OpenWrite(wav_path)) {
        LOGE("OpenWrite failed (%s)", wav_path);
        Deinit();
        return false;
    }

    if (!writer_.Open(&file_, sample_rate_hz, bits, channels)) {
        LOGE("Open failed");
        Deinit();
        return false;
    }

    open_          = true;
    error_         = false;
    total_bytes_   = 0;
    sd_data_bytes_ = 0;

    return true;
}

/**
 * @brief 録音を終了し、WAVをクローズしてファイルシステムをアンマウントする
 */
void SdBppRecorder::Deinit()
{
    if (open_) {
        // 残りを吐く。失敗してもCloseは試す
        if (!FlushChunk()) {
            error_ = true;
        }

        writer_.Close();
        open_ = false;
    }

    file_.Close();
    fs_.Unmount();

    sd_data_bytes_ = 0;
    total_bytes_   = 0;
    error_         = false;
}

/**
 * @brief ステージングバッファの内容をSDに書き出す
 */
bool SdBppRecorder::FlushChunk()
{
    if (sd_data_bytes_ == 0) {
        return true;
    }

    const int32_t bytes_written = writer_.WriteData(sd_buf_, static_cast<uint32_t>(sd_data_bytes_));
    if (bytes_written < 0) {
        LOGE("WriteData failed");
        return false;
    }

    // 部分書き込みもあり得る前提でチェック
    if (static_cast<size_t>(bytes_written) != sd_data_bytes_) {
        LOGE("Partial write req=%u wrote=%d", (unsigned)sd_data_bytes_, bytes_written);
        return false;
    }

    total_bytes_ += static_cast<uint32_t>(sd_data_bytes_);
    sd_data_bytes_ = 0;

    return true;
}

/**
 * @brief プールから1BPPを取得してWAVへ追記する
 *
 * @param[in,out] pool 供給先プール
 * @retval true  1BPPを消費できた（または n==0 で実質何も書かずに返却した）
 * @retval false open_でない、rx_pool が null、または AcquireForRx できない（未到着）
 *
 * @note 制約: この関数内にエラーログ以外仕込むの禁止
 *        - 低レイテンシ/高頻度呼び出し前提のため、重い処理を入れない意図。
 */
bool SdBppRecorder::ConsumeOneBpp(AudioBppPool *pool)
{
    if (!open_ || !pool) {
        return false;
    }

    AudioBppPool::Bpp bpp;
    if (!pool->AcquireForRx(&bpp)) {
        // データ未到着
        return false;
    }

    const int32_t bpp_bytes = (bpp.valid_bytes > 0) ? bpp.valid_bytes : 0;
    if (bpp_bytes > 0) {
        // 溢れる前にFlush
        if (sd_data_bytes_ + static_cast<size_t>(bpp_bytes) > kSDChunk) {
            if (!FlushChunk()) {
                error_ = true;
                pool->Recycle(bpp);
                return false;
            }
        }

        std::memcpy(&sd_buf_[sd_data_bytes_], bpp.addr, static_cast<size_t>(bpp_bytes));
        sd_data_bytes_ += static_cast<size_t>(bpp_bytes);

        if (sd_data_bytes_ == kSDChunk) {
            if (!FlushChunk()) {
                error_ = true;
                pool->Recycle(bpp);
                return false;
            }
        }
    }

    pool->Recycle(bpp);

    return true;
}

}  // namespace module
}  // namespace core0
