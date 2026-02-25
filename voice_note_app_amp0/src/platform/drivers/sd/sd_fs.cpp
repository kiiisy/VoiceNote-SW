/**
 * @file sd_fs.cpp
 * @brief sd_fs.hの実装
 */

// 自ヘッダー
#include "sd_fs.h"

// 標準ライブラリ
#include <limits>
#include <utility>

// Vitisライブラリ
extern "C" {
#include "ff.h"
}

// プロジェクトライブラリ
#include "logger_core.h"

namespace core0 {
namespace platform {

struct SdFs::Impl
{
    FATFS fs{};
};

struct FatFsFile::Impl
{
    FIL file{};
};

// SdFs

SdFs::SdFs() noexcept = default;

SdFs::~SdFs() noexcept { Unmount(); }
SdFs::SdFs(SdFs &&other) noexcept : impl_(std::move(other.impl_)), mounted_(other.mounted_) { other.mounted_ = false; }

SdFs &SdFs::operator=(SdFs &&other) noexcept
{
    if (this == &other) {
        return *this;
    }

    // 既存インスタンスが管理しているmount状態を先に解放する
    Unmount();
    impl_          = std::move(other.impl_);
    mounted_       = other.mounted_;
    other.mounted_ = false;

    return *this;
}

/**
 * @brief PImpl実体を確保する
 *
 * @retval true 実体が利用可能
 * @retval false 実体確保に失敗
 */
bool SdFs::EnsureImpl() noexcept
{
    if (impl_) {
        return true;
    }

    Impl *p = new (std::nothrow) Impl();
    if (!p) {
        LOGE("SdFs: OOM");
        return false;
    }

    impl_.reset(p);

    return true;
}

/**
 * @brief FatFsをマウントする
 *
 * @retval true 成功（既にマウント済みを含む）
 * @retval false 失敗
 */
bool SdFs::Mount() noexcept
{
    if (mounted_) {
        return true;
    }

    if (!EnsureImpl()) {
        return false;
    }

    const FRESULT result = f_mount(&impl_->fs, "", 1);
    if (result != FR_OK) {
        LOGE("SdFs: f_mount failed (result=%d)", (int)result);
        return false;
    }

    mounted_ = true;

    return true;
}

/**
 * @brief FatFsをアンマウントする
 */
void SdFs::Unmount() noexcept
{
    if (!mounted_) {
        // マウント前に確保だけされた場合を想定
        impl_.reset();
        return;
    }

    const FRESULT result = f_mount(nullptr, "", 0);
    if (result != FR_OK) {
        LOGE("SdFs: unmount failed (result=%d)", (int)result);
        return;
    }

    mounted_ = false;
    impl_.reset();
}

/**
 * @brief ディレクトリエントリを1件ずつコールバックへ渡す
 *
 * @param path 対象ディレクトリパス
 * @param max_entries 走査上限（0で無制限）
 * @param on_entry エントリ受信コールバック（false返却で早期終了）
 * @retval true 走査成功（途中停止含む）
 * @retval false 失敗
 */
bool SdFs::ForEachDirEntry(const char *path, uint16_t max_entries,
                           const std::function<bool(const DirEntryInfo &)> &on_entry) noexcept
{
    if (!mounted_) {
        LOGE("SdFs: ForEachDirEntry while not mounted");
        return false;
    }

    if (!path || path[0] == '\0') {
        LOGE("SdFs: ForEachDirEntry invalid path");
        return false;
    }

    DIR     dir{};
    FRESULT res = f_opendir(&dir, path);
    if (res != FR_OK) {
        LOGE("SdFs: f_opendir failed (result=%d) path=%s", (int)res, path);
        return false;
    }

    uint16_t count = 0;
    for (;;) {
        FILINFO fi{};
        res = f_readdir(&dir, &fi);
        if (res != FR_OK) {
            LOGE("SdFs: f_readdir failed (result=%d)", (int)res);
            f_closedir(&dir);
            return false;
        }
        if (fi.fname[0] == '\0') {
            break;
        }

        // fi.fnameの寿命は次のf_readdir呼び出しまで。
        const DirEntryInfo entry{fi.fname, static_cast<uint32_t>(fi.fsize), (fi.fattrib & AM_DIR) != 0};
        if (on_entry && !on_entry(entry)) {
            break;
        }

        ++count;
        if (max_entries != 0 && count >= max_entries) {
            break;
        }
    }

    f_closedir(&dir);

    return true;
}

// FatFsFile

FatFsFile::FatFsFile() noexcept = default;
FatFsFile::~FatFsFile() noexcept { Close(); }

FatFsFile::FatFsFile(FatFsFile &&other) noexcept : impl_(std::move(other.impl_)), opened_(other.opened_)
{
    // ムーブ元のIsOpen()が真を返さないように状態を落とす
    other.opened_ = false;
}

FatFsFile &FatFsFile::operator=(FatFsFile &&other) noexcept
{
    if (this == &other) {
        return *this;
    }

    // 既存ハンドルを閉じてから所有を引き継ぐ
    Close();
    impl_         = std::move(other.impl_);
    opened_       = other.opened_;
    other.opened_ = false;

    return *this;
}

/**
 * @brief PImpl実体を確保する
 *
 * @retval true 実体が利用可能
 * @retval false 実体確保に失敗
 */
bool FatFsFile::EnsureImpl() noexcept
{
    if (impl_) {
        return true;
    }

    Impl *p = new (std::nothrow) Impl();
    if (!p) {
        LOGE("FatFsFile: OOM");
        return false;
    }

    impl_.reset(p);

    return true;
}

/**
 * @brief パス妥当性チェック
 */
bool FatFsFile::ValidatePath(const char *path) noexcept
{
    if (!path || path[0] == '\0') {
        return false;
    }

    for (const char *p = path; *p; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (c < 0x20) {
            return false;
        }
    }

    return true;
}

/**
 * @brief モード指定でファイルを開く
 *
 * @param path ファイルパス
 * @param mode FatFs のオープンモード
 * @retval true 成功
 * @retval false 失敗
 */
bool FatFsFile::OpenImpl(const char *path, uint8_t mode) noexcept
{
    if (!ValidatePath(path)) {
        LOGE("FatFsFile: invalid path");
        return false;
    }

    // Open前に既存ハンドルを必ず閉じる
    Close();

    if (!EnsureImpl()) {
        return false;
    }

    const FRESULT result = f_open(&impl_->file, path, (BYTE)mode);

    opened_ = (result == FR_OK);
    if (!opened_) {
        LOGE("FatFsFile: f_open failed (result=%d) path=%s", (int)result, path);
        impl_.reset();
        return false;
    }

    return true;
}

/**
 * @brief 書き込み用でファイルを開く
 *
 * @param path ファイルパス
 * @retval true 成功
 * @retval false 失敗
 */
bool FatFsFile::OpenWrite(const char *path) noexcept { return OpenImpl(path, (uint8_t)(FA_WRITE | FA_CREATE_ALWAYS)); }

/**
 * @brief 読み込み用でファイルを開く
 *
 * @param path ファイルパス
 * @retval true 成功
 * @retval false 失敗
 */
bool FatFsFile::OpenRead(const char *path) noexcept { return OpenImpl(path, (uint8_t)(FA_READ | FA_OPEN_EXISTING)); }

/**
 * @brief ファイルを閉じる
 */
void FatFsFile::Close() noexcept
{
    if (opened_ && impl_) {
        const FRESULT result = f_close(&impl_->file);
        if (result != FR_OK) {
            LOGE("FatFsFile: f_close failed (result=%d)", (int)result);
        }
    }

    // f_closeの成否に関わらずローカル状態はclosedに戻す
    opened_ = false;
    impl_.reset();
}

/**
 * @brief ファイルからデータを読み込む
 *
 * @param buffer 読み込み先バッファ
 * @param bytes 読み込み要求バイト数
 * @return 読めたバイト数（0以上）/ 失敗時 -1
 */
int FatFsFile::Read(void *buffer, uint32_t bytes) noexcept
{
    if (!opened_ || !impl_) {
        LOGE("FatFsFile: Read while not open");
        return -1;
    }

    if (!buffer || bytes == 0) {
        LOGE("FatFsFile: Read invalid args");
        return -1;
    }

    UINT bytes_read = 0;

    const FRESULT result = f_read(&impl_->file, buffer, (UINT)bytes, &bytes_read);
    if (result != FR_OK) {
        LOGE("FatFsFile: f_read failed (result=%d)", (int)result);
        return -1;
    }

    return (int)bytes_read;
}

/**
 * @brief ファイルへデータを書き込む
 *
 * @param data 書き込み元バッファ
 * @param bytes 書き込み要求バイト数
 * @return 書けたバイト数（0以上）/ 失敗時 -1
 */
int FatFsFile::Write(const void *data, uint32_t bytes) noexcept
{
    if (!opened_ || !impl_) {
        LOGE("FatFsFile: Write while not open");
        return -1;
    }

    if (!data || bytes == 0) {
        LOGE("FatFsFile: Write invalid args");
        return -1;
    }

    UINT bytes_written = 0;

    const FRESULT result = f_write(&impl_->file, data, (UINT)bytes, &bytes_written);
    if (result != FR_OK) {
        LOGE("FatFsFile: f_write failed (result=%d)", (int)result);
        return -1;
    }

    return (int)bytes_written;
}

/**
 * @brief 書き込み内容をメディアへ同期する
 *
 * @retval true  成功
 * @retval false 失敗
 */
bool FatFsFile::Flush() noexcept
{
    if (!opened_ || !impl_) {
        LOGE("FatFsFile: Flush while not open");
        return false;
    }

    const FRESULT result = f_sync(&impl_->file);
    if (result != FR_OK) {
        LOGE("FatFsFile: f_sync failed (result=%d)", (int)result);
        return false;
    }

    return true;
}

/**
 * @brief 現在のファイル位置を取得する
 *
 * @return 現在位置（byte）。未オープン時は 0。
 */
uint32_t FatFsFile::Tell() const noexcept
{
    if (!opened_ || !impl_) {
        LOGE("FatFsFile: Tell while not open");
        return 0;
    }

    const FSIZE_t fptr = impl_->file.fptr;
    if (fptr > static_cast<FSIZE_t>(std::numeric_limits<uint32_t>::max())) {
        LOGE("FatFsFile: Tell overflow (fptr=%llu)", static_cast<unsigned long long>(fptr));
        return std::numeric_limits<uint32_t>::max();
    }

    return static_cast<uint32_t>(fptr);
}

/**
 * @brief ファイル位置を移動する
 *
 * @param pos 移動先（byte）
 * @retval true  成功
 * @retval false 失敗
 */
bool FatFsFile::Seek(uint32_t pos) noexcept
{
    if (!opened_ || !impl_) {
        LOGE("FatFsFile: Seek while not open");
        return false;
    }

    const FRESULT result = f_lseek(&impl_->file, (FSIZE_t)pos);
    if (result != FR_OK) {
        LOGE("FatFsFile: f_lseek failed (result=%d)", (int)result);
        return false;
    }

    return true;
}

}  // namespace platform
}  // namespace core0
