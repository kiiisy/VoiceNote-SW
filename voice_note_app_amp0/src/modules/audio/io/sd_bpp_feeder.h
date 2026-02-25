/**
 * @file sd_bpp_feeder.h
 * @brief SDカード上のWAVファイルをメモリプールへ供給する
 *
 * SDカード（FatFs）からWAVファイルを読み出し、メモリプールに対して
 * 常に1BPP単位で供給する。
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "sd_fs.h"
#include "wav_reader.h"

namespace core0 {
namespace module {

class AudioBppPool;

/**
 * @class SdBppFeeder
 * @brief SD上のWAVをメモリプールへ供給するクラス
 */
class SdBppFeeder
{
public:
    SdBppFeeder() = default;
    ~SdBppFeeder();

    SdBppFeeder(const SdBppFeeder &)            = delete;
    SdBppFeeder &operator=(const SdBppFeeder &) = delete;

    bool Init(const char *wav_path);
    void Deinit();

    bool ProvideBpp(AudioBppPool *pool);

    uint32_t GetBytesPerSec() const;
    bool     GetEof() const { return eof_; }
    uint32_t GetTotalBytes() const { return total_bytes_; };

private:
    platform::SdFs      fs_{};      // SDカードファイルシステム
    platform::FatFsFile file_{};    // オープン中のファイルハンドル
    WavReader           reader_{};  // WAVパーサ/リーダ

    bool     ready_{false};    // Init済みか否か
    bool     eof_{false};      // EOF到達フラグ
    uint32_t total_bytes_{0};  // WAVデータの総バイト数
};

}  // namespace module
}  // namespace core0
