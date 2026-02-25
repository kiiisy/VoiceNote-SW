/**
 * @file reg_core.h
 * @brief 32bitメモリマップドレジスタ操作ユーティリティ
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xil_io.h"

namespace core1 {
namespace platform {

/**
 * @brief 32bitレジスタを表すラッパクラス
 */
class Reg32
{
public:
    explicit Reg32(uintptr_t addr) : addr_(addr) {}

    /**
     * @brief レジスタ値を読み出す
     *
     * @return 32bit レジスタ値
     */
    inline uint32_t Read() const { return Xil_In32(addr_); }

    /**
     * @brief レジスタ値を書き込む
     *
     * @param v 書き込む値
     */
    inline void Write(uint32_t v) const { Xil_Out32(addr_, v); }

    /**
     * @brief 指定ビットをセットする
     *
     * @param mask セットするビットマスク
     */
    inline void SetBits(uint32_t mask) const { Write(Read() | mask); }

    /**
     * @brief 指定ビットをクリアする
     *
     * @param mask クリアするビットマスク
     */
    inline void ClrBits(uint32_t mask) const { Write(Read() & ~mask); }

    /**
     * @brief マスク指定で値を代入する
     *
     * @param mask 書き換え対象ビットマスク
     * @param value_shifted すでにシフト済みの値
     */
    inline void Assign(uint32_t mask, uint32_t value_shifted) const
    {
        uint32_t data = Read();
        data          = (data & ~mask) | (value_shifted & mask);
        Write(data);
    }

    /**
     * @brief ビットフィールドを書き込む
     *
     * @param mask フィールドのビットマスク
     * @param shift ビットシフト量
     * @param value フィールド値（未シフト）
     */
    inline void WriteField(uint32_t mask, uint8_t shift, uint32_t value) const { Assign(mask, (value << shift)); }

private:
    uintptr_t addr_;  // レジスタの物理アドレス
};

/**
 * @brief レジスタブロック管理クラス
 */
class RegCore
{
public:
    explicit RegCore(uintptr_t base) : base_(base) {}

    /**
     * @brief 指定オフセットのレジスタを取得する
     *
     * @param offset ベースアドレスからのオフセット
     * @return Reg32 オブジェクト
     */
    inline Reg32 Reg(uint32_t offset) const { return Reg32(base_ + offset); }

    /**
     * @brief ベースアドレスを取得する
     *
     * @return ベースアドレス
     */
    inline uintptr_t base() const { return base_; }

protected:
    uintptr_t base_;  // レジスタブロックのベースアドレス
};

}  // namespace platform
}  // namespace core1
