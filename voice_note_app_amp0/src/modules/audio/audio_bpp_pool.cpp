/**
 * @file audio_bpp_pool.cpp
 * @brief AudioBppPoolの実装
 */

// 自ヘッダー
#include "audio_bpp_pool.h"

// 標準ライブラリ
#include <cstring>
#include <new>

// プロジェクトライブラリ
#include "logger_core.h"
#include "utility.h"

namespace core0 {
namespace module {

/**
 * @struct AudioBppPool::Impl
 * @brief AudioBppPool の内部実装（PImpl）
 *
 * - リングバッファ（ID のキュー）を保持
 * - BPP のメタ情報（valid_bytes）を保持
 * - base_addr と bpp_size からアドレス算出を行う
 */
struct AudioBppPool::Impl
{
    /**
     * @struct Ring
     * @brief 固定長リングバッファ（int32_t ID 用）
     *
     * cap は Init() で設定され、0..cap-1 の範囲で Push/Pop する想定。
     * buf の物理サイズは kMax 固定だが、実際の有効容量は cap で決まる。
     */
    struct Ring
    {
        static constexpr int32_t kMax = 128;  // リングの最大保持数（bpp_count の上限）

        int32_t buf[kMax]{};  // ID 保持領域
        int32_t head = 0;     // 読み出し位置
        int32_t tail = 0;     // 書き込み位置
        int32_t len  = 0;     // 現在の要素数
        int32_t cap  = 0;     // 有効容量（<= kMax）

        /**
         * @brief リングを初期化する
         * @param[in] capacity 使用容量（<= kMax）
         */
        void Init(int32_t capacity)
        {
            cap  = capacity;
            head = tail = len = 0;
        }

        /**
         * @brief 次インデックスを計算する
         */
        static inline int32_t Next(int32_t i, int32_t cap) { return (i + 1) % cap; }

        /**
         * @brief 末尾へ追加する
         * @retval true  成功
         * @retval false 満杯
         */
        bool Push(int32_t v)
        {
            if (len >= cap) {
                return false;
            }
            buf[tail] = v;
            tail      = Next(tail, cap);
            ++len;
            return true;
        }

        /**
         * @brief 先頭から取り出す
         * @param[out] out 取り出した値
         * @retval true  成功
         * @retval false 空
         */
        bool Pop(int32_t *out)
        {
            if (len <= 0) {
                return false;
            }
            *out = buf[head];
            head = Next(head, cap);
            --len;
            return true;
        }
        bool Empty() const { return len == 0; }
        bool Full() const { return len >= cap; }
    };

    uintptr_t base_addr = 0;                 // BPPプールの先頭物理アドレス（Non-cacheable DDR を想定）
    int32_t   bpp_size  = kBppSize;          // 1BPPのサイズ [byte]
    int32_t   bpp_count = kDefaultBppCount;  // BPP の総数（0..bpp_count-1 が有効 ID）

    Ring free_q;   // 未使用 BPP の ID キュー（Alloc で Pop / Release・Recycle で Push）
    Ring ready_q;  // 待ちキュー（TX: 送信待ち / RX: 受信済み待ち）

    int32_t valid_bytes[Ring::kMax] = {0};  // BPP ごとの有効データ長 [byte]（id で参照）

    int32_t min_level = 0x7fffffff;  // ready_q の最小水位（運用中の最小 len を記録）

    /**
     * @brief BPP の先頭アドレスを算出する
     *
     * @param[in] id BPP ID（0..bpp_count-1）
     * @return BPP 先頭アドレス
     */
    inline void *AddrOf(int32_t id) const
    {
        return reinterpret_cast<void *>(base_addr + static_cast<uintptr_t>(id) * bpp_size);
    }
};

AudioBppPool::~AudioBppPool() { Deinit(); }

/**
 * @brief プールを初期化する
 *
 * @param[in] base_addr Non-cacheable DDR 上のバッファ先頭アドレス
 * @param[in] bpp_size  1 BPP のサイズ（基本は kBppSize と一致する必要がある）
 * @param[in] bpp_count BPP 数（<= Ring::kMax）
 *
 * @retval true  成功
 * @retval false 失敗（サイズ不一致、範囲外、OOM など）
 */
bool AudioBppPool::Init(uintptr_t base_addr, int32_t bpp_size, int32_t bpp_count)
{
    LOG_SCOPE();

    if (impl_) {
        return true;
    }

    if (bpp_size != kBytesPerPeriod) {
        return false;
    }

    if (bpp_count <= 0 || bpp_count > AudioBppPool::Impl::Ring::kMax) {
        return false;
    }

    impl_ = new (std::nothrow) Impl();
    if (!impl_) {
        LOGE("AudioBppPool::Init: OOM (heap exhausted)");
        return false;
    }

    impl_->base_addr = base_addr;
    impl_->bpp_size  = bpp_size;
    impl_->bpp_count = bpp_count;

    impl_->free_q.Init(bpp_count);
    impl_->ready_q.Init(bpp_count);

    // 初期 free キューに 0..(count-1) を投入
    for (int32_t i = 0; i < bpp_count; ++i) {
        impl_->valid_bytes[i] = 0;
        impl_->free_q.Push(i);
    }

    impl_->min_level = impl_->free_q.len;
    underrun_count_  = 0;
    overrun_count_   = 0;
    return true;
}

/**
 * @brief プールを終了する
 *
 * 内部実装（Impl）を解放し、未初期化状態へ戻す。
 */
void AudioBppPool::Deinit()
{
    if (!impl_) {
        return;
    }

    delete impl_;
    impl_ = nullptr;
}

/**
 * @brief 空き BPP を 1 つ割り当てる（書き込み側）
 *
 * free_q から 1 つ Pop し、out に addr/id を設定して返す。
 * 書き込み完了後は Commit() を呼び、ready_q へ移す。
 *
 * @param[out] out 取得した BPP
 * @retval true  成功
 * @retval false 失敗（未初期化、空き無しなど）
 */
bool AudioBppPool::Alloc(Bpp *out)
{
    if (!impl_ || !out) {
        LOGE("Object not found");
        return false;
    }

    InterruptGuard guard;

    int32_t id;
    if (!impl_->free_q.Pop(&id)) {
        LOGE("Memory pool POP failed");
        // 空きなし
        return false;
    }

    out->id          = id;
    out->addr        = impl_->AddrOf(id);
    out->valid_bytes = 0;

    return true;
}

/**
 * @brief 書き込み完了した BPP を送信待ちキューへ積む（TX）
 *
 * valid_bytes は 0..bpp_size を想定する（EOF シグナル等で 0 を許容する設計）。
 * ready_q が満杯の場合は、最古をドロップして最新を入れる（overrun としてカウント）。
 *
 * @param[in] bpp         Commit 対象 BPP
 * @param[in] valid_bytes 有効データ長 [byte]
 */
void AudioBppPool::Commit(const Bpp &bpp, int32_t valid_bytes)
{
    if (!impl_) {
        return;
    }

    // valid_bytes は 0..bpp_size（EOFシグナルで 0 も許容）
    if (bpp.id < 0 || bpp.id >= impl_->bpp_count) {
        LOGE("BPP id is invalid");
        return;
    }
    if (valid_bytes < 0 || valid_bytes > impl_->bpp_size) {
        LOGE("valid_bytes out of range: id=%d valid=%d max=%d", bpp.id, valid_bytes, impl_->bpp_size);
        return;
    }

    impl_->valid_bytes[bpp.id] = valid_bytes;
    InterruptGuard guard;

    if (!impl_->ready_q.Push(bpp.id)) {
        // 送信待ちが満杯 → あり得ない（設計上 TX は free→tx_ready の一方向）。
        // ひとまず最古を落として overrun としてカウント。
        int32_t drop;
        impl_->ready_q.Pop(&drop);
        ++overrun_count_;
        (void)drop;
        impl_->ready_q.Push(bpp.id);
    }
}

/**
 * @brief 送信（再生）用に BPP を 1 つ取得する（TX）
 *
 * ready_q から Pop し、out に addr/valid_bytes/id を設定して返す。
 * 空の場合は underrun をカウントする（再生側の飢餓）。
 *
 * @param[out] out 取得した BPP
 * @retval true  成功
 * @retval false 失敗（未初期化、データ無し）
 */
bool AudioBppPool::AcquireForTx(Bpp *out)
{
    if (!impl_ || !out) {
        return false;
    }

    InterruptGuard guard;

    int32_t id;
    if (!impl_->ready_q.Pop(&id)) {
        // アンダーラン
        ++underrun_count_;
        return false;
    }

    out->id           = id;
    out->addr         = impl_->AddrOf(id);
    out->valid_bytes  = impl_->valid_bytes[id];
    int32_t cur_level = impl_->ready_q.len;
    if (cur_level < impl_->min_level) {
        impl_->min_level = cur_level;
    }

    return true;
}

/**
 * @brief 送信完了した BPP をプールへ返却する（TX）
 *
 * valid_bytes をクリアし、free_q へ戻す。
 *
 * @param[in] bpp 返却する BPP
 */
void AudioBppPool::Release(const Bpp &bpp)
{
    if (!impl_) {
        return;
    }

    if (bpp.id < 0 || bpp.id >= impl_->bpp_count) {
        return;
    }

    impl_->valid_bytes[bpp.id] = 0;
    InterruptGuard guard;

    // 受信系と共有しない free キューに戻す
    if (!impl_->free_q.Push(bpp.id)) {
        ++overrun_count_;
        LOGE("free_q push failed in Release: id=%d", bpp.id);
    }
}

/**
 * @brief 受信済み BPP を上位処理が取得する（RX）
 *
 * ready_q から Pop して返す。
 * （※TX と同じ ready_q を使うため、TX と RX を同一インスタンスで同時運用する設計には注意）
 *
 * @param[out] out 取得した BPP
 * @retval true  成功
 * @retval false 失敗（未初期化、データ無し）
 */
bool AudioBppPool::AcquireForRx(Bpp *out)
{
    if (!impl_ || !out) {
        return false;
    }

    InterruptGuard guard;

    int32_t id;
    if (!impl_->ready_q.Pop(&id)) {
        return false;  // データなし
    }

    out->id          = id;
    out->addr        = impl_->AddrOf(id);
    out->valid_bytes = impl_->valid_bytes[id];

    return true;
}

/**
 * @brief 上位処理が使い終わった BPP を返却する（RX）
 *
 * valid_bytes をクリアし、free_q へ戻す。
 *
 * @param[in] bpp 返却する BPP
 */
void AudioBppPool::Recycle(const Bpp &bpp)
{
    if (!impl_) {
        return;
    }

    if (bpp.id < 0 || bpp.id >= impl_->bpp_count) {
        return;
    }

    impl_->valid_bytes[bpp.id] = 0;
    InterruptGuard guard;

    if (!impl_->free_q.Push(bpp.id)) {
        ++overrun_count_;
        LOGE("free_q push failed in Recycle: id=%d", bpp.id);
    }
}

/**
 * @brief 受信 ISR 等が「受信完了した BPP」を ready_q に積む（RX）
 *
 * ready_q が満杯の場合は最古をドロップして最新を入れる（overrun としてカウント）。
 *
 * @param[in] bpp         対象 BPP（id が有効であること）
 * @param[in] valid_bytes 有効データ長 [byte]
 */
void AudioBppPool::Produce(const Bpp &bpp, int32_t valid_bytes)
{
    if (!impl_) {
        return;
    }

    if (bpp.id < 0 || bpp.id >= impl_->bpp_count) {
        return;
    }
    if (valid_bytes < 0 || valid_bytes > impl_->bpp_size) {
        LOGE("valid_bytes out of range: id=%d valid=%d max=%d", bpp.id, valid_bytes, impl_->bpp_size);
        return;
    }

    impl_->valid_bytes[bpp.id] = valid_bytes;
    InterruptGuard guard;

    if (!impl_->ready_q.Push(bpp.id)) {
        // 上位が取り遅れ → 最古を捨てて最新追随
        int32_t drop;
        impl_->ready_q.Pop(&drop);
        ++overrun_count_;
        (void)drop;
        impl_->ready_q.Push(bpp.id);
    }
}

/**
 * @brief 現在のバッファ保持数を返す
 *
 * @return buffered数（未初期化なら 0）
 */
int32_t AudioBppPool::GetBufferedCount() const
{
    if (!impl_) {
        return 0;
    }

    InterruptGuard guard;

    return impl_->ready_q.len;  // 待ち水位を返す
}

/**
 * @brief 運用中の最小バッファ保持数を返す
 *
 * TX 運用時は「どれだけ枯れたか」の指標になる。
 *
 * @return 最小buffered数（未初期化なら 0）
 */
int32_t AudioBppPool::GetMinBufferedCount() const
{
    if (!impl_) {
        return 0;
    }

    InterruptGuard guard;

    return impl_->min_level;
}

/**
 * @brief プールのベースアドレスを取得する
 * @return base_addr（未初期化なら 0）
 */
uintptr_t AudioBppPool::GetBaseAddr() const { return impl_ ? impl_->base_addr : 0; }

}  // namespace module
}  // namespace core0
