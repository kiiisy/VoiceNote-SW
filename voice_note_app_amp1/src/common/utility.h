#include "xiltimer.h"

/**
 * @brief 現在時刻をms単位で取得する
 */
uint32_t GetTimeMs()
{
    XTime t;
    XTime_GetTime(&t);

    const uint64_t cps = (uint64_t)COUNTS_PER_SECOND;
    return (uint32_t)((t * 1000ULL) / cps);
}
