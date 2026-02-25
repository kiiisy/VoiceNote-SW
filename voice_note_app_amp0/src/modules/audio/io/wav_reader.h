/**
 * @file wav_reader.h
 * @brief WAV(PCM)ファイルからPCMデータを読み出す
 *
 * WavReaderはFatFsFile を借用してWAV(PCM)のヘッダ解析を行い、
 * dataチャンクのPCMバイト列をReadSome()で逐次読み出す。
 *
 * 対応:
 * - RIFF/WAVE
 * - fmtチャンク（PCM: audio_format == 1）
 * - dataチャンク
 *
 * 非対応（現状）:
 * - PCM以外（IEEE float など）
 * - data以外の拡張チャンク内容の解釈（未知チャンクはスキップのみ）
 */
#pragma once

// 標準ライブラリ
#include <cstdint>
#include <functional>

namespace core0 {

namespace platform {
class FatFsFile;
}

namespace module {

/**
 * @class WavReader
 * @brief WAV(PCM)のヘッダ解析とデータ読出しを提供するクラス
 */
class WavReader
{
public:
    struct WavFileInfo
    {
        char     path[80]{};
        uint32_t size{0};
        uint32_t sample_rate{0};
        uint16_t bits_per_sample{0};
        uint16_t channels{0};
    };

    WavReader() = default;
    ~WavReader();

    WavReader(const WavReader &)            = delete;
    WavReader &operator=(const WavReader &) = delete;

    bool Open(platform::FatFsFile *file);
    void Close();

    static bool EnumerateWavFiles(const char *dir_path, uint16_t max_files,
                                  const std::function<bool(const WavFileInfo &)> &on_file);

    int32_t ReadSome(void *dst, uint32_t bytes);

    uint32_t GetSampleRate() const { return sample_rate_; }
    uint16_t GetBitsPerSample() const { return bits_per_sample_; }
    uint16_t GetChannels() const { return channels_; }
    uint32_t GetDataBytesTotal() const { return data_bytes_total_; }
    uint32_t GetDataBytesRemaining() const { return data_bytes_remaining_; }

private:
    platform::FatFsFile *file_{nullptr};  // 借用ポインタ（非所有）

    uint32_t sample_rate_{0};           // fmt.sample_rate
    uint16_t bits_per_sample_{0};       // fmt.bits_per_sample
    uint16_t channels_{0};              // fmt.channels
    uint32_t data_bytes_total_{0};      // dataチャンクの総バイト数
    uint32_t data_bytes_remaining_{0};  // 未読の残量
};

}  // namespace module
}  // namespace core0
