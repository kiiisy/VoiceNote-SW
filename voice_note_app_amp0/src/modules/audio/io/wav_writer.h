/**
 * @file wav_writer.h
 * @brief WAV(PCM)ファイルを書き出す
 *
 * WavWriterはFatFsFileを借用し、WAV(PCM)ヘッダを書き出した後に
 * WriteData()でdata チャンクへ連続追記する。
 *
 * 制約:
 * - WAVのRIFFヘッダは32bitサイズフィールドのため、data_bytesが4GBを超える録音は非対応。
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {

namespace platform {
class FatFsFile;
}

namespace module {

/**
 * @class WavWriter
 * @brief WAV(PCM)のヘッダ生成とPCMデータ追記を提供するクラス
 */
class WavWriter
{
public:
    WavWriter() = default;
    ~WavWriter();

    WavWriter(const WavWriter &)            = delete;
    WavWriter &operator=(const WavWriter &) = delete;

    bool Open(platform::FatFsFile *file, uint32_t sample_rate_hz, uint16_t bits, uint16_t channels);
    bool Close();

    int32_t WriteData(const void *data, uint32_t bytes);

    uint32_t GetBytesWritten() const { return data_bytes_; }
    bool     IsOpen() const { return open_; }

private:
    bool WriteAll(platform::FatFsFile *f, const void *p, uint32_t bytes);
    bool SeekAbs(platform::FatFsFile *f, uint32_t pos);

    // RIFF.size = 4("WAVE") + (8 + 16)[fmt chunk] + (8 + data_bytes)[data chunk]
    static constexpr uint32_t kRiffOverheadBytes = 36;
    static constexpr uint32_t kMaxDataBytes      = 0xFFFFFFFFu - kRiffOverheadBytes;
    static constexpr uint32_t kFmtBytes          = 16;

    platform::FatFsFile *file_{nullptr};  // 借用ポインタ（非所有）

    uint32_t start_pos_{0};      // Open開始時点のファイル位置（ロールバック用）
    uint32_t data_size_pos_{0};  // data.size フィールド位置（パッチ用）
    uint32_t riff_size_pos_{4};  // RIFF.size フィールド位置（常に先頭から+4の想定）
    uint32_t data_bytes_{0};     // dataチャンクへ実際に書き込めたバイト数の累計
    bool     open_{false};
};

}  // namespace module
}  // namespace core0
