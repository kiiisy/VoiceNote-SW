/**
 * @file core_init.cpp
 * @brief core_initの実装
 */

// 自ヘッダー
#include "core_init.h"

// プロジェクトライブラリ
#include "mailbox.h"

namespace core0 {

/**
 * @brief IPC 共有メモリ領域を初期化する
 *
 * Core1起動前に1度だけ実行
 */
void InitSharedIpcRegion()
{
    // 共有IPC領域はOCM Highの固定アドレスを直接参照する
    auto *s = core::ipc::GetShared();

    // magic/version は「初期化済み・互換バージョン」の判定用メタ情報
    s->magic   = 0x4D504941u;  // "MPIA"
    s->version = 1;

    // cmd: core0 -> core1, evt: core1 -> core0
    // head/tailを同時に0に戻して、両リングを空状態にする
    s->cmd.head = s->cmd.tail = 0;
    s->evt.head = s->evt.tail = 0;
}

}  // namespace core0
