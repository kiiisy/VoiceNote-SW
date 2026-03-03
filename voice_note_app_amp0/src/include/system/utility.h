#pragma once

#include <cstdint>

#include "xil_exception.h"
#include "xiltimer.h"

namespace core0 {

/**
 * @brief 割り込み状態を保存して復元するガード
 *
 * ctor で割り込みを禁止し、dtor で「元が有効だった場合のみ」再有効化する。
 */
class InterruptGuard final
{
public:
    InterruptGuard() noexcept
    {
#if defined(__arm__)
        uint32_t cpsr = 0;
        __asm__ volatile("mrs %0, cpsr" : "=r"(cpsr));
        irq_was_enabled_ = ((cpsr & 0x80u) == 0u);
#endif
        Xil_ExceptionDisable();
    }

    ~InterruptGuard() noexcept
    {
        if (irq_was_enabled_) {
            Xil_ExceptionEnable();
        }
    }

private:
    bool irq_was_enabled_{false};
};

/**
 * @brief 現在時刻をms単位で取得する
 */
inline uint32_t GetTimeMs()
{
    XTime t;
    XTime_GetTime(&t);

    const uint64_t cps = (uint64_t)COUNTS_PER_SECOND;
    return (uint32_t)((t * 1000ULL) / cps);
}

}  // namespace core0
