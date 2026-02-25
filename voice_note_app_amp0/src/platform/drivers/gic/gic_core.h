/**
 * @file gic_core.h
 * @brief GIC（割り込みコントローラ）管理クラス
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// Vitisライブラリ
#include "xil_exception.h"
#include "xscugic.h"
#include "xstatus.h"

namespace core0 {
namespace platform {

/**
 * @brief GIC管理クラス
 */
class GicCore final
{
public:
    GicCore() = default;
    ~GicCore() { Deinit(); }

    /**
     * @brief CPUルーティング
     */
    enum class CpuTarget : uint8_t
    {
        Cpu0 = 0x00,
        Cpu1 = 0x02,
    };

    /**
     * @brief GIC初期化設定
     */
    struct Config
    {
        uint32_t  base_addr{};
        CpuTarget target_cpu{CpuTarget::Cpu0};
    };

    /**
     * @brief 割り込み優先度
     */
    enum class GicPriority : uint8_t
    {
        Normal = 0xA0,
    };

    /**
     * @brief トリガ種別
     */
    enum class GicTrigger : uint8_t
    {
        LevelHigh = 0x01,
        Rising    = 0x03,
    };

    static GicCore &GetInstance()
    {
        static GicCore inst;
        return inst;
    }

    XStatus Init(const Config &cfg);
    void    Deinit();

    XStatus Attach(uint32_t int_id, Xil_InterruptHandler handler, void *ref, GicPriority priority, GicTrigger trigger);
    void    Detach(uint32_t int_id);
    void    Enable(uint32_t int_id);
    void    Disable(uint32_t int_id);

private:
    XScuGic gic_{};       // GICの実体
    bool    run_{false};  // 起動中か否か

    Config cfg_{};  // 初期化設定
};

}  // namespace platform
}  // namespace core0
