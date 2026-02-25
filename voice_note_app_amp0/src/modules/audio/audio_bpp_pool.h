/**
 * @file audio_bpp_pool.h
 * @brief Audio BPP（Bytes Per Period）固定長バッファプール
 *
 * 固定長 BPP（Bytes Per Period）バッファをプール管理する。
 * 想定用途:
 * - TX（再生）: 上位が書き込んだ BPP を「送信待ちキュー」として保持し、DMA/I2S 側が取り出す
 * - RX（録音）: ISR 等が BPP を「受信済みキュー」として積み、上位が取り出す
 *
 * 設計前提:
 * - Non-cacheable DDR 域を使用する前提（キャッシュ操作は行わない）
 * - 割り込みコンテキストとタスク/メインコンテキストが同一キューを触るため、
 *   内部で割り込み禁止（InterruptGuard）によりクリティカルセクションを作る
 */
#pragma once

// 標準ライブラリ
#include <cstddef>
#include <cstdint>

// プロジェクトライブラリ
#include "common.h"

namespace core0 {
namespace module {

/**
 * @class AudioBppPool
 * @brief 固定長 BPP バッファのプール管理クラス
 */
class AudioBppPool final
{
public:
    /**
     * @struct Bpp
     * @brief 1BPPバッファのハンドル
     */
    struct Bpp
    {
        void   *addr        = nullptr;  // 先頭アドレス
        int32_t valid_bytes = 0;        // 有効データ長
        int32_t id          = -1;       // 内部ID
    };

    static constexpr int32_t kBppSize         = kBytesPerPeriod;
    static constexpr int32_t kBppsPerChunk    = kPeriodsPerChunk;
    static constexpr int32_t kDefaultBppCount = kPeriodsPerChunk;

    AudioBppPool() = default;
    ~AudioBppPool();

    bool      Init(uintptr_t base_addr, int32_t bpp_size = kBppSize, int32_t bpp_count = kDefaultBppCount);
    void      Deinit();
    uintptr_t GetBaseAddr() const;

    // TX 系 API
    bool Alloc(Bpp *out);
    void Commit(const Bpp &bpp, int32_t valid_bytes);
    bool AcquireForTx(Bpp *out);
    void Release(const Bpp &bpp);

    // RX 系 API
    bool AcquireForRx(Bpp *out);
    void Recycle(const Bpp &bpp);
    void Produce(const Bpp &bpp, int32_t valid_bytes);

    // 監視
    int32_t GetBufferedCount() const;
    int32_t GetMinBufferedCount() const;

private:
    struct Impl;
    Impl    *impl_{nullptr};      // 実装隠蔽（リングバッファ、メタ情報、ベースアドレス等）
    uint32_t underrun_count_{0};  // アンダーラン発生回数
    uint32_t overrun_count_{0};   // オーバーラン発生回数
};

}  // namespace module
}  // namespace core0
