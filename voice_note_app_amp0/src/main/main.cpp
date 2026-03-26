// プロジェクトライブラリ
#include "addr_map.h"
#include "amp_hw.h"
#include "app_server.h"
#include "audio_format.h"
#include "core_boot.h"
#include "core_init.h"
#include "gic_core.h"
#include "gpio_pl.h"
#include "i2s_rx_core.h"
#include "i2s_tx_core.h"
#include "irq_map.h"
#include "logger_core.h"
#include "memory_map.h"
#include "mmu_utility.h"
#include "system.h"

int main()
{
    LOG_SCOPE();

    // --------------------------------------------
    // CPUブート & IPCサーバー起動
    // --------------------------------------------
    core::ipc::InitAmpOcmSharedSafe();

    core::ipc::EnableGicDistributorGroup01();

    core0::InitSharedIpcRegion();

    core0::BootCpu1(core0::kCpu1StartAddr);

    auto *shared = core::ipc::GetShared();

    core0::ipc::AppServer server(shared);

    // --------------------------------------------
    // 非キャッシュ領域化
    // --------------------------------------------
    {
        uintptr_t pool_bytes  = core0::kBytesPerPeriod * core0::kPeriodsPerChunk;
        size_t    ring_bytes  = 2 * core0::kPeriodsPerHalfBytes;
        size_t    total_bytes = pool_bytes + ring_bytes;
        MakeRegionNonCacheable(reinterpret_cast<void *>(core0::kDmaBufBasePhys), total_bytes);
    }

    // --------------------------------------------
    // 各初期化 & 処理実行
    // --------------------------------------------
    auto     &gic = core0::platform::GicCore::GetInstance();
    const int gic_init_status =
        gic.Init({.base_addr = core0::kGicBaseAddr, .target_cpu = core0::platform::GicCore::CpuTarget::Cpu0});
    if (gic_init_status != XST_SUCCESS) {
        LOGE("Gic init failed");
        return -1;
    }

    core0::platform::GpioPl::Config cfg{core0::kPlGpioBaseAddr, core0::kPlGpioIrqId,
                                        core0::platform::GpioPl::ChType::kChBtn,
                                        core0::platform::GpioPl::ChType::kChLed};
    core0::platform::GpioPl         gpio_pl(cfg);

    auto &i2s_rx = core0::platform::I2sRx::GetInstance();
    auto &i2s_tx = core0::platform::I2sTx::GetInstance();

    core0::app::System sys(server, i2s_tx, i2s_rx);

    const bool sys_init_ok = sys.Init();
    if (!sys_init_ok) {
        LOGE("System init failed");
        return -1;
    }

    sys.SetHandlers();
    sys.Run();

    return 0;
}
