/**
 * @file ddr_audio_buffer.h
 * @brief DDR線形バッファ（録音保持/再生読出し）
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core0 {
namespace module {

class DdrAudioBuffer final
{
public:
    /**
     * @brief 内部状態モード
     */
    enum class Mode : uint8_t
    {
        kUninit = 0,
        kIdle,
        kRecord,
        kPlay,
    };

    DdrAudioBuffer() = default;

    bool Init(uintptr_t base_addr, uint32_t capacity_bytes);
    void Deinit();
    void ResetForRecord();
    bool Append(const void *src, uint32_t bytes);
    void FinalizeRecord();
    bool ResetForPlay();

    uint32_t ReadSome(void *dst, uint32_t bytes);

    Mode     GetMode() const { return mode_; }
    uint32_t GetCapacityBytes() const { return capacity_bytes_; }
    uint32_t GetValidBytes() const { return valid_bytes_; }
    uint32_t GetWritePos() const { return write_pos_; }
    uint32_t GetReadPos() const { return read_pos_; }

    uint32_t  GetRemainingBytes() const;
    uintptr_t GetBaseAddr() const { return base_addr_; }

private:
    uintptr_t base_addr_{0};
    uint32_t  capacity_bytes_{0};
    uint32_t  valid_bytes_{0};
    uint32_t  write_pos_{0};
    uint32_t  read_pos_{0};
    Mode      mode_{Mode::kUninit};
};

}  // namespace module
}  // namespace core0
