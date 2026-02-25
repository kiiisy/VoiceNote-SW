/**
 * @file codec_if.h
 * @brief オーディオコーデック抽象インターフェース
 *
 * オーディオコーデック（例: SSM2603, WM8731 等）を
 * 実装差し替え可能にするための純粋仮想インターフェース。
 *
 * - I2C 等の制御バスの詳細は各実装クラスに隠蔽する
 * - core0 側の上位ロジックは本インターフェースのみに依存する
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {
namespace module {

/**
 * @class AudioCodec
 * @brief オーディオコーデック抽象インターフェース
 *
 * 入出力音量制御、ミュート制御など、
 * オーディオコーデックに共通する最小限の操作を定義する。
 *
 * 実装クラスは以下を想定:
 * - I2C 初期化およびレジスタ設定
 * - 出力音量 / 入力音量のパーセンテージ指定
 * - ミュート制御
 */
class AudioCodec
{
public:
    virtual ~AudioCodec() = default;

    // ハードのI2Cアドレスなど、必要なら引数で初期化
    virtual bool Init()                                   = 0;
    virtual bool SetOutVolume(uint8_t Lpct, uint8_t Rpct) = 0;  // 0..100
    virtual bool SetInVolume(uint8_t Lpct, uint8_t Rpct)  = 0;  // 0..100
    virtual void Mute(bool en)                            = 0;
};
}  // namespace module
}  // namespace core0
