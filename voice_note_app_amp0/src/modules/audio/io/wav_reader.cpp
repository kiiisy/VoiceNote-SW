/**
 * @file wav_reader.cpp
 * @brief WavReader実装
 */

// 自ヘッダー
#include "wav_reader.h"

// 標準ライブラリ
#include <cstdio>
#include <cstring>

// プロジェクトライブラリ
#include "logger_core.h"
#include "sd_fs.h"

namespace {

static constexpr uint32_t kFmtMinBytes   = 16;   // fmt chunk 基本サイズ
static constexpr uint32_t kChunkHdrBytes = 8;    // id[4] + size[4]
static constexpr uint32_t kMaxChunkScans = 256;  // 探索上限（壊れたファイルでのハング防止）

/**
 * @brief 4文字ID(FourCC) の一致判定
 *
 * @param[in] id WAV 内の 4byte ID
 * @param[in] s  比較文字列（4文字想定）
 * @retval true  一致
 * @retval false 不一致
 */
inline bool IsFourCCEqual(const char id[4], const char *s)
{
    return id[0] == s[0] && id[1] == s[1] && id[2] == s[2] && id[3] == s[3];
}

// WAV(RIFF)はチャンクデータを2byte境界に揃える（奇数サイズのとき1byte pad）
inline uint32_t PadToWord(uint32_t size) { return size + (size & 1u); }

void BuildPath(char *dst, size_t dst_size, const char *dir, const char *name)
{
    if (!dst || dst_size == 0) {
        return;
    }

    if (!dir || dir[0] == '\0') {
        std::snprintf(dst, dst_size, "%s", name ? name : "");
        return;
    }

    const size_t len = std::strlen(dir);
    if (len > 0 && dir[len - 1] == '/') {
        std::snprintf(dst, dst_size, "%s%s", dir, name ? name : "");
    } else {
        std::snprintf(dst, dst_size, "%s/%s", dir, name ? name : "");
    }
}

}  // namespace

namespace core0 {
namespace module {

WavReader::~WavReader() { Close(); }

bool WavReader::EnumerateWavFiles(const char *dir_path, uint16_t max_files,
                                  const std::function<bool(const WavFileInfo &)> &on_file)
{
    if (!dir_path || dir_path[0] == '\0') {
        return false;
    }

    platform::SdFs fs{};
    if (!fs.Mount()) {
        return false;
    }

    uint16_t   produced = 0;
    const bool ok       = fs.ForEachDirEntry(dir_path, 0, [&](const platform::SdFs::DirEntryInfo &entry) -> bool {
        if (entry.is_dir) {
            return true;
        }

        WavFileInfo info{};
        BuildPath(info.path, sizeof(info.path), dir_path, entry.name);
        info.size = entry.size;

        platform::FatFsFile file{};
        if (!file.OpenRead(info.path)) {
            return true;
        }

        WavReader wav{};
        if (!wav.Open(&file)) {
            file.Close();
            return true;
        }

        info.sample_rate     = wav.GetSampleRate();
        info.bits_per_sample = wav.GetBitsPerSample();
        info.channels        = wav.GetChannels();

        wav.Close();
        file.Close();

        if (on_file && !on_file(info)) {
            return false;
        }

        ++produced;
        if (max_files != 0 && produced >= max_files) {
            return false;
        }
        return true;
    });

    fs.Unmount();

    return ok;
}

/**
 * @brief WAV(PCM)をオープンし、ヘッダ解析を行う
 *
 * @param[in] file Open済みのFatFsFile
 * @retval true  成功（以後 ReadSome() が data チャンクから読み出す）
 * @retval false 失敗（ファイル形式不正、読み込み失敗、PCM でない等）
 */
bool WavReader::Open(platform::FatFsFile *file)
{
    LOG_SCOPE();

    Close();

    if (!file || !file->IsOpen()) {
        LOGE("Invalid file (null or not open)");
        return false;
    }

    file_ = file;

    // 再利用時のファイル位置を必ず先頭に戻す
    if (!file_->Seek(0)) {
        LOGE("Seek(0) failed");
        Close();
        return false;
    }

    struct RiffHeader
    {
        char     riff[4];
        uint32_t size;
        char     wave[4];
    } rh{};

    if (file_->Read(&rh, sizeof(rh)) != static_cast<int32_t>(sizeof(rh))) {
        LOGE("Read RIFF header failed");
        return false;
    }

    if (!IsFourCCEqual(rh.riff, "RIFF") || !IsFourCCEqual(rh.wave, "WAVE")) {
        LOGE("Not RIFF/WAVE");
        return false;
    }

    // fmt(最低16Bytes)を読む（それ以上はスキップ）
    struct WavFmt
    {
        uint16_t audio_format;
        uint16_t channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
    } fmt{};

    bool     got_fmt    = false;
    bool     got_data   = false;
    uint32_t data_bytes = 0;

    uint32_t prev_pos = file_->Tell();
    for (uint32_t scans = 0; scans < kMaxChunkScans; ++scans) {

        struct Chunk
        {
            char     id[4];
            uint32_t size;
        } ch{};

        if (file_->Read(&ch, sizeof(ch)) != static_cast<int32_t>(sizeof(ch))) {
            LOGE("Read chunk header failed");
            return false;
        }

        const uint32_t after_pos = file_->Tell();
        if (after_pos <= prev_pos) {
            // Seekが壊れてる/同じ場所を回ってるなどの異常
            LOGE("Chunk scan stuck (pos did not advance)");
            Close();
            return false;
        }
        prev_pos = after_pos;

        if (IsFourCCEqual(ch.id, "fmt ")) {
            if (ch.size < 16) {
                LOGE("Fmt chunk too small (size=%u)", (unsigned)ch.size);
                return false;
            }

            if (file_->Read(&fmt, 16) != 16) {
                LOGE("Read fmt body failed");
                return false;
            }

            // 余りをスキップ（パディング含む）
            const uint32_t payload_extra = ch.size - kFmtMinBytes;
            const uint32_t skip          = PadToWord(payload_extra);
            if (skip != 0) {
                const uint32_t current = file_->Tell();
                if (!file_->Seek(current + skip)) {
                    LOGE("Skip fmt extra failed");
                    Close();
                    return false;
                }
            }
            got_fmt = true;

        } else if (IsFourCCEqual(ch.id, "data")) {
            // dataチャンクは本体の先頭がこの直後なのでスキップ無しでbreak
            data_bytes = ch.size;
            got_data   = true;
            break;

        } else {
            // 未知チャンクはsize(+pad)だけスキップ
            const uint32_t skip    = PadToWord(ch.size);
            const uint32_t current = file_->Tell();
            if (!file_->Seek(current + skip)) {
                LOGE("Skip unknown chunk failed");
                Close();
                return false;
            }
        }

        // 位置が増えているかの最終チェック
        const uint32_t now_pos = file_->Tell();
        if (now_pos <= prev_pos) {
            LOGE("Chunk scan stuck after skip (pos did not advance)");
            Close();
            return false;
        }
        prev_pos = now_pos;
    }

    // 妥当性チェック
    if (!got_fmt || !got_data) {
        LOGE("Missing fmt or data");
        Close();
        return false;
    }

    if (fmt.audio_format != 1) {
        LOGE("Not PCM (format=%u)", (unsigned)fmt.audio_format);
        Close();
        return false;
    }

    if (fmt.channels == 0 || fmt.channels > 2) {
        LOGE("Unsupported channels=%u", (unsigned)fmt.channels);
        Close();
        return false;
    }

    if (fmt.bits_per_sample != 8 && fmt.bits_per_sample != 16 && fmt.bits_per_sample != 24 &&
        fmt.bits_per_sample != 32) {
        LOGE("Unsupported bits_per_sample=%u", (unsigned)fmt.bits_per_sample);
        Close();
        return false;
    }

    const uint16_t expected_align = (uint16_t)(fmt.channels * (fmt.bits_per_sample / 8u));
    if (fmt.block_align == 0 || fmt.block_align != expected_align) {
        LOGE("Invalid block_align=%u expected=%u", (unsigned)fmt.block_align, (unsigned)expected_align);
        Close();
        return false;
    }

    sample_rate_          = fmt.sample_rate;
    bits_per_sample_      = fmt.bits_per_sample;
    channels_             = fmt.channels;
    data_bytes_total_     = data_bytes;
    data_bytes_remaining_ = data_bytes;

    return true;
}

/**
 * @brief 状態をリセットする（ファイルはCloseしない）
 */
void WavReader::Close()
{
    file_                 = nullptr;
    sample_rate_          = 0;
    bits_per_sample_      = 0;
    channels_             = 0;
    data_bytes_total_     = 0;
    data_bytes_remaining_ = 0;
}

/**
 * @brief PCMデータを最大bytes分読み出す
 *
 * @param[out] data   読み出し先
 * @param[in]  bytes 最大読み出しバイト数
 * @return 読み出したバイト数（>0）、EOF で 0、失敗で -1
 *
 * @note 制約: この関数内にエラーログ以外仕込むの禁止
 *        - 低レイテンシ/高頻度呼び出し前提のため、重い処理を入れない意図。
 */
int32_t WavReader::ReadSome(void *data, uint32_t bytes)
{
    if (!file_) {
        LOGE("ReadSome while not open");
        return -1;
    }

    if (!data || bytes == 0) {
        // bytes == 0 は何も読まないなのでEOF扱いにしてもいいが、
        // 呼び出しバグ検出するなら -1 にしても良いかも。とりあえず0で。
        return 0;
    }

    if (data_bytes_remaining_ == 0) {
        return 0;
    }

    uint32_t want = (bytes < data_bytes_remaining_) ? bytes : data_bytes_remaining_;

    int32_t bytes_read = file_->Read(data, want);
    if (bytes_read < 0) {
        LOGE("f_read failed");
        return -1;
    }

    data_bytes_remaining_ -= static_cast<uint32_t>(bytes_read);

    return bytes_read;
}

}  // namespace module
}  // namespace core0
