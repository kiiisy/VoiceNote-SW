/**
 * @file ddr_audio_buffer.cpp
 * @brief DDR線形バッファ実装
 */

// 自ヘッダー
#include "ddr_audio_buffer.h"

// 標準ライブラリ
#include <cstring>

namespace core0 {
namespace module {

/**
 * @brief バッファを初期化する
 *
 * @param[in] base_addr      バッファ先頭アドレス
 * @param[in] capacity_bytes バッファ容量[byte]
 * @retval true  成功
 * @retval false 失敗（引数不正）
 */
bool DdrAudioBuffer::Init(uintptr_t base_addr, uint32_t capacity_bytes)
{
    if (base_addr == 0u || capacity_bytes == 0u) {
        return false;
    }

    base_addr_      = base_addr;
    capacity_bytes_ = capacity_bytes;
    valid_bytes_    = 0;
    write_pos_      = 0;
    read_pos_       = 0;
    mode_           = Mode::kIdle;
    return true;
}

/**
 * @brief 未初期化状態へ戻す
 */
void DdrAudioBuffer::Deinit()
{
    base_addr_      = 0;
    capacity_bytes_ = 0;
    valid_bytes_    = 0;
    write_pos_      = 0;
    read_pos_       = 0;
    mode_           = Mode::kUninit;
}

/**
 * @brief 録音モードへ遷移し、書き込み状態をクリアする
 */
void DdrAudioBuffer::ResetForRecord()
{
    if (mode_ == Mode::kUninit) {
        return;
    }

    // 新規録音開始時は前回内容を破棄して先頭から書き直す
    valid_bytes_ = 0;
    write_pos_   = 0;
    read_pos_    = 0;
    mode_        = Mode::kRecord;
}

/**
 * @brief 録音データを末尾へ追記する
 *
 * @param[in] src   追記元データ
 * @param[in] bytes 追記サイズ[byte]
 * @retval true  成功
 * @retval false 失敗（モード不正、引数不正、容量不足）
 */
bool DdrAudioBuffer::Append(const void *src, uint32_t bytes)
{
    if (mode_ != Mode::kRecord || src == nullptr) {
        return false;
    }

    if (bytes == 0u) {
        return true;
    }

    if (write_pos_ > capacity_bytes_) {
        return false;
    }

    // 末尾を超える書き込みは受け付けない（呼び出し側で停止判断）
    const uint32_t remain = capacity_bytes_ - write_pos_;
    if (bytes > remain) {
        return false;
    }

    std::memcpy(reinterpret_cast<void *>(base_addr_ + write_pos_), src, bytes);
    write_pos_ += bytes;
    return true;
}

/**
 * @brief 録音完了を確定する
 *
 * write_pos を valid_bytes として確定し、再生準備状態へ戻す。
 */
void DdrAudioBuffer::FinalizeRecord()
{
    if (mode_ != Mode::kRecord) {
        return;
    }

    // 実際に書けた長さだけを再生対象として確定する
    valid_bytes_ = write_pos_;
    read_pos_    = 0;
    mode_        = Mode::kIdle;
}

/**
 * @brief 再生モードへ遷移し、読出し位置を先頭へ戻す
 *
 * @retval true  成功
 * @retval false 未初期化、または内部状態不整合
 */
bool DdrAudioBuffer::ResetForPlay()
{
    if (mode_ == Mode::kUninit) {
        return false;
    }

    if (valid_bytes_ == 0u) {
        // 空データでも再生モード遷移自体は成功扱いにする
        read_pos_ = 0;
        mode_     = Mode::kPlay;
        return true;
    }

    if (valid_bytes_ > capacity_bytes_) {
        return false;
    }

    read_pos_ = 0;
    mode_     = Mode::kPlay;
    return true;
}

/**
 * @brief 再生データを読み出す
 *
 * @param[out] dst   読み出し先
 * @param[in]  bytes 読み出し要求サイズ[byte]
 * @return 実際に読み出したサイズ[byte]
 */
uint32_t DdrAudioBuffer::ReadSome(void *dst, uint32_t bytes)
{
    if (mode_ != Mode::kPlay || dst == nullptr || bytes == 0u) {
        return 0u;
    }

    if (read_pos_ > valid_bytes_) {
        return 0u;
    }

    // 要求サイズと残量の小さい方だけを返す（末尾は部分読み）
    const uint32_t remain  = valid_bytes_ - read_pos_;
    const uint32_t to_copy = (bytes < remain) ? bytes : remain;
    if (to_copy == 0u) {
        return 0u;
    }

    std::memcpy(dst, reinterpret_cast<const void *>(base_addr_ + read_pos_), to_copy);
    read_pos_ += to_copy;
    return to_copy;
}

/**
 * @brief 再生で未読の残バイト数を返す
 *
 * @return 残バイト数（再生モード以外は0）
 */
uint32_t DdrAudioBuffer::GetRemainingBytes() const
{
    if (mode_ != Mode::kPlay || read_pos_ >= valid_bytes_) {
        return 0u;
    }
    return valid_bytes_ - read_pos_;
}

}  // namespace module
}  // namespace core0
