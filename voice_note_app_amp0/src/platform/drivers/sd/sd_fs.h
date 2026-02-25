/**
 * @file sd_fs.h
 * @brief FatFsを扱う（マウント/ファイル）
 *
 * - `ff.h`依存をcppに隔離（PImpl）
 * - 例外なし前提のため `new(std::nothrow)` を使用
 * - コピー禁止 / ムーブ可
 */
#pragma once

// 標準ライブラリ
#include <cstdint>
#include <functional>
#include <memory>

namespace core0 {
namespace platform {

/**
 * @brief FatFsのf_mount / f_unmountを扱うクラス
 *
 * - copy禁止（ファイルシステム実体の複製はできない）
 * - move可
 */
class SdFs final
{
public:
    struct DirEntryInfo
    {
        // このポインタはForEachDirEntryのコールバック呼び出し中のみ有効
        const char *name{nullptr};
        uint32_t    size{0};
        bool        is_dir{false};
    };

    SdFs() noexcept;
    ~SdFs() noexcept;

    SdFs(const SdFs &)            = delete;
    SdFs &operator=(const SdFs &) = delete;

    SdFs(SdFs &&) noexcept;
    SdFs &operator=(SdFs &&) noexcept;

    bool Mount() noexcept;
    void Unmount() noexcept;
    bool ForEachDirEntry(const char *path, uint16_t max_entries,
                         const std::function<bool(const DirEntryInfo &)> &on_entry) noexcept;

    bool mounted() const noexcept { return mounted_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
    bool                  mounted_{false};

    bool EnsureImpl() noexcept;
};

/**
 * @brief FatFsのFILを扱うクラス
 *
 * - copy禁止（同一ハンドル複製は危険）
 * - move可
 */
class FatFsFile final
{
public:
    FatFsFile() noexcept;
    ~FatFsFile() noexcept;

    FatFsFile(const FatFsFile &)            = delete;
    FatFsFile &operator=(const FatFsFile &) = delete;

    FatFsFile(FatFsFile &&) noexcept;
    FatFsFile &operator=(FatFsFile &&) noexcept;

    bool OpenWrite(const char *path) noexcept;
    bool OpenRead(const char *path) noexcept;
    void Close() noexcept;

    bool IsOpen() const noexcept { return opened_; }

    int      Read(void *dst, uint32_t bytes) noexcept;         // 戻り値: 読めたバイト数 / -1
    int      Write(const void *src, uint32_t bytes) noexcept;  // 戻り値: 書けたバイト数 / -1
    bool     Flush() noexcept;
    uint32_t Tell() const noexcept;
    bool     Seek(uint32_t pos) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
    bool                  opened_{false};

    bool        EnsureImpl() noexcept;
    static bool ValidatePath(const char *path) noexcept;

    bool OpenImpl(const char *path, uint8_t mode) noexcept;
};

}  // namespace platform
}  // namespace core0
