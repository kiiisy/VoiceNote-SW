// プロジェクトライブラリ
#include "amp_hw.h"
#include "app_server.h"
#include "audio_engine.h"
#include "audio_ipc_controller.h"
#include "board_io.h"
#include "common.h"
#include "core_boot.h"
#include "core_init.h"
#include "gic_core.h"
#include "gpio_pl.h"
#include "i2s_rx_core.h"
#include "i2s_tx_core.h"
#include "logger_core.h"
#include "mmu_utility.h"

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

    auto *s = core::ipc::GetShared();

    core0::ipc::AppServer server(s);

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
    auto &gic = core0::platform::GicCore::GetInstance();
    gic.Init({.base_addr = core0::kGicBaseAddr, .target_cpu = core0::platform::GicCore::CpuTarget::Cpu0});

    core0::platform::GpioPl::Config cfg{core0::kPlGpioBaseAddr, XPS_FPGA5_INT_ID,
                                        core0::platform::GpioPl::ChType::kChBtn,
                                        core0::platform::GpioPl::ChType::kChLed};
    core0::platform::GpioPl         gpio_pl(cfg);

    auto &i2s_rx = core0::platform::I2sRx::GetInstance();
    auto &i2s_tx = core0::platform::I2sTx::GetInstance();

    core0::app::AudioEngine sys(i2s_tx, i2s_rx);

    if (!sys.Init()) {
        LOGE("System Initialization Failed");
        return -1;
    }

    core0::app::AudioIpcController app(server, sys);
    app.SetHandlers();

    app.Run();

    return 0;
}
