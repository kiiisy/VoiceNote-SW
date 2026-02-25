/**
 * @file wav_writer.cpp
 * @brief WavWriterの実装
 */

// 自ヘッダー
#include "wav_writer.h"

// 標準ライブラリ
#include <cstring>

// プロジェクトライブラリ
#include "logger_core.h"
#include "sd_fs.h"

namespace core0 {
namespace module {

WavWriter::~WavWriter() { Close(); }

bool WavWriter::WriteAll(platform::FatFsFile *file, const void *p, uint32_t bytes)
{
    return file && (file->Write(p, bytes) == static_cast<int32_t>(bytes));
}

bool WavWriter::SeekAbs(platform::FatFsFile *file, uint32_t pos) { return file && file->Seek(pos); }

/**
 * @brief WAV(PCM)の書き込みを開始する
 *
 * @param[in] file           Open済みのFatFsFile
 * @param[in] sample_rate_hz サンプルレート
 * @param[in] bits           量子化ビット数
 * @param[in] channels       チャンネル数
 * @retval true  成功（以後 WriteData() で PCM 追記可能）
 * @retval false 失敗（ヘッダ書き込み失敗など）
 */
bool WavWriter::Open(platform::FatFsFile *file, uint32_t sample_rate_hz, uint16_t bits, uint16_t channels)
{
    Close();

    if (!file || !file->IsOpen()) {
        LOGE("Invalid file");
        return false;
    }

    if (sample_rate_hz == 0 || channels == 0) {
        LOGE("Invalid params (sample_rate_hz=%u channels=%u)", (unsigned)sample_rate_hz, (unsigned)channels);
        return false;
    }

    if (!(bits == 8 || bits == 16 || bits == 24 || bits == 32)) {
        LOGE("Unsupported bits=%u", (unsigned)bits);
        return false;
    }

    file_       = file;
    open_       = true;
    data_bytes_ = 0;

    // ロールバック
    start_pos_ = static_cast<uint32_t>(file_->Tell());

    struct RIFF
    {
        char     riff[4];
        uint32_t size;  // 後でパッチ
        char     wave[4];
    } riff{
        {'R', 'I', 'F', 'F'},
        0, {'W', 'A', 'V', 'E'}
    };

    struct Fmt
    {
        char     id[4];
        uint32_t size;  // 16(PCM)
        uint16_t fmt;   // 1=PCM
        uint16_t ch_;
        uint32_t sr_;
        uint32_t br;     // byte rate = sr*ch*(bits/8)
        uint16_t align;  // block align = ch*(bits/8)
        uint16_t bps;    // bits per sample
    } fmt{
        {'f', 'm', 't', ' '},
        kFmtBytes,
        1,
        channels,
        sample_rate_hz,
        sample_rate_hz * channels * (bits / 8),
        static_cast<uint16_t>(channels * (bits / 8)),
        bits
    };

    struct Data
    {
        char     id[4];
        uint32_t size;  // 後でパッチ
    } data{
        {'d', 'a', 't', 'a'},
        0
    };

    // ヘッダ書き込み（失敗時はロールバックして閉じる）
    auto RollbackFail = [&](const char *msg) -> bool {
        LOGE("%s", msg);

        // 可能な範囲で位置を戻す
        (void)SeekAbs(file_, start_pos_);
        open_          = false;
        file_          = nullptr;
        start_pos_     = 0;
        data_size_pos_ = 0;
        data_bytes_    = 0;
        return false;
    };

    // 書き込み（エラー時はロールバック）
    if (!WriteAll(file_, &riff, sizeof(riff))) {
        return RollbackFail("Write RIFF failed");
    }

    if (!WriteAll(file_, &fmt, sizeof(fmt))) {
        return RollbackFail("Write fmt failed");
    }

    // data.sizeの位置を記録（dataヘッダ書く前の現在位置+4）
    const uint32_t data_hdr_pos = static_cast<uint32_t>(file_->Tell());
    data_size_pos_              = data_hdr_pos + 4;

    if (!WriteAll(file_, &data, sizeof(data))) {
        return RollbackFail("Write data header failed");
    }

    return true;
}

/**
 * @brief WAVの書き込みを終了し、ヘッダのサイズを確定してFlushする
 */
bool WavWriter::Close()
{
    if (!open_ || !file_) {
        open_          = false;
        file_          = nullptr;
        start_pos_     = 0;
        data_size_pos_ = 0;
        data_bytes_    = 0;
        return true;
    }

    bool result = true;

    // 現在位置退避
    const uint32_t current = static_cast<uint32_t>(file_->Tell());

    // data.sizeをパッチ
    if (data_size_pos_ == 0) {
        LOGE("data_size_pos_ is 0");
        result = false;
    } else {
        if (!SeekAbs(file_, data_size_pos_)) {
            LOGE("Seek data.size failed");
            result = false;
        } else {
            const uint32_t data_size = data_bytes_;
            if (!WriteAll(file_, &data_size, sizeof(data_size))) {
                LOGE("Patch data.size failed");
                result = false;
            }
        }
    }

    // RIFF.sizeをパッチ（36 + data_bytes）
    {
        const uint32_t riff_size = kRiffOverheadBytes + data_bytes_;
        if (!SeekAbs(file_, riff_size_pos_)) {
            LOGE("Seek RIFF.size failed");
            result = false;
        } else {
            if (!WriteAll(file_, &riff_size, sizeof(riff_size))) {
                LOGE("Patch RIFF.size failed");
                result = false;
            }
        }
    }

    // 元の末尾へ戻す
    if (!SeekAbs(file_, current)) {
        LOGE("Seek back to end failed");
        result = false;
    }

    // Flush
    if (!file_->Flush()) {
        LOGE("Flush failed");
        result = false;
    }

    // 状態クリア
    open_          = false;
    file_          = nullptr;
    start_pos_     = 0;
    data_size_pos_ = 0;
    data_bytes_    = 0;

    return result;
}

/**
 * @brief PCMデータをdataチャンクへ追記する
 *
 * @param[in] data 追記するPCMバイト列
 * @param[in] bytes 追記するバイト数
 * @return 実際に書けたバイト数（>=0）、失敗時 -1
 */
int32_t WavWriter::WriteData(const void *data, uint32_t bytes)
{
    if (!open_ || !file_ || !data || bytes == 0) {
        return -1;
    }

    // 4GB越えを事前に拒否する
    if (data_bytes_ > kMaxDataBytes) {
        LOGE("Already exceeded max");
        return -1;
    }
    if (bytes > (kMaxDataBytes - data_bytes_)) {
        LOGE("Would exceed 4GB limit (cur=%u add=%u max=%u)", (unsigned)data_bytes_, (unsigned)bytes,
             (unsigned)kMaxDataBytes);
        return -1;
    }

    // FatFsFileが一括書き込み長を返す前提
    const int32_t bytes_written = file_->Write(data, bytes);
    if (bytes_written < 0) {
        LOGE("Write failed");
        return -1;
    }

    if (bytes_written > 0) {
        data_bytes_ += static_cast<uint32_t>(bytes_written);
    }

    return bytes_written;
}

}  // namespace module
}  // namespace core0
