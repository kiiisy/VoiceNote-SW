/**
 * @file sd_bpp_recorder.h
 * @brief メモリプールのBPPをSDへ書き出すレコーダ
 *
 * メモリプールから到着した BPPを取り出し、
 * SDカード上のWAVファイルへ追記保存する。
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "sd_fs.h"
#include "wav_writer.h"

namespace core0 {
namespace module {

class AudioBppPool;

/**
 * @class SdBppRecorder
 * @brief メモリプールをSDのWAVファイルへ保存するクラス
 */
class SdBppRecorder
{
public:
    SdBppRecorder() = default;
    ~SdBppRecorder();

    SdBppRecorder(const SdBppRecorder &)            = delete;
    SdBppRecorder &operator=(const SdBppRecorder &) = delete;

    bool Init(const char *wav_path, uint32_t sample_rate_hz, uint16_t bits, uint16_t channels);
    void Deinit();

    bool ConsumeOneBpp(AudioBppPool *pool);

    uint32_t GetTotalBytes() const { return total_bytes_; }

private:
    bool FlushChunk();

    platform::SdFs      fs_{};      // SDカードファイルシステム
    platform::FatFsFile file_{};    // オープン中のファイルハンドル
    WavWriter           writer_{};  // WAVパーサ/ライタ

    bool     open_{false};  // Init済みか否か
    bool     error_{false};
    uint32_t total_bytes_{0};  // 実際にWAVへ書けた累計

    static constexpr size_t kSDChunk = 64 * 1024;  // SDへの書き込み単位
    alignas(32) uint8_t sd_buf_[kSDChunk];         // 書き込み前のステージングバッファ
    size_t sd_data_bytes_{0};                      // sd_buf_内の有効データ量
};

}  // namespace module
}  // namespace core0
