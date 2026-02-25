/**
 * @file codec_provider.h
 * @brief AudioCodec 実装切り替え用ファサード
 *
 * AudioCodec の実体を隠蔽し、システム全体から
 * 「現在有効なコーデック実装」を取得するためのクラス。
 *
 * - 明示的に Set() されていない場合は既定実装を返す
 * - 既定実装は codec_ssm2603.cpp 側で定義される
 */
#pragma once

// プロジェクトライブラリ
#include "codec_if.h"

namespace core0 {
namespace module {

/**
 * @class CodecProvider
 * @brief AudioCodec 実装取得用ファサード（Singleton）
 *
 * 実際の AudioCodec 実装（SSM2603 など）を
 * アプリケーション側から切り離すための薄いラッパー。
 *
 * 想定用途:
 * - ボード差し替え時のコーデック変更
 * - テスト用ダミー実装への切り替え
 */
class CodecProvider
{
public:
    static CodecProvider &GetInstance()
    {
        static CodecProvider inst;
        return inst;
    }

    void        Set(AudioCodec *impl) { impl_ = impl; }
    AudioCodec &Get() { return *EnsureDefault(); }

private:
    CodecProvider() = default;

    /**
     * @brief 既定の AudioCodec 実装を取得する
     *
     * impl_ が nullptr の場合に呼ばれ、
     * 既定実装（例: SSM2603）を生成・保持して返す。
     *
     * 実装は codec_ssm2603.cpp 側に定義される。
     *
     * @return AudioCodec 実装へのポインタ（nullptr にはならない想定）
     */
    AudioCodec *EnsureDefault();

    AudioCodec *impl_ = nullptr;  // 現在使用中の AudioCodec 実装
};
}  // namespace module
}  // namespace core0
