// Vitisライブラリ
#include "sleep.h"

// プロジェクトライブラリ
#include "amp_hw.h"
#include "app_server.h"
#include "audio_ipc_controller.h"
#include "audio_notification.h"
#include "board_io.h"
#include "codec_provider.h"
#include "common.h"
#include "core_boot.h"
#include "core_init.h"
#include "gic_core.h"
#include "gpio_pl.h"
#include "i2c_pl_core.h"
#include "i2s_rx_core.h"
#include "i2s_tx_core.h"
#include "ipc_msg.h"
#include "logger_core.h"
#include "mailbox.h"
#include "mmu_utility.h"
#include "ssm2603.h"
#include "audio_engine.h"

// 汚いけどいったんここに置いておく
#define DMA_BUF_BASE 0x10000000U

int main()
{
    LOG_SCOPE();

    core::ipc::InitAmpOcmSharedSafe();

    core::ipc::EnableGicDistributorGroup01();

    core0::InitSharedIpcRegion();

    const uint32_t CPU1_START_ADDR = 0x08000000u;
    LOGI("[cpu0] boot cpu1 @0x%08x\r\n", CPU1_START_ADDR);
    core0::BootCpu1(CPU1_START_ADDR);

    auto *s = core::ipc::GetShared();

    core0::ipc::AppServer server(s);

    // 非キャッシュ化
    uintptr_t pool_bytes  = core0::kBytesPerPeriod * core0::kPeriodsPerChunk;
    size_t    ring_bytes  = 2 * core0::kPeriodsPerHalfBytes;
    size_t    total_bytes = pool_bytes + ring_bytes;
    MakeRegionNonCacheable(reinterpret_cast<void *>(DMA_BUF_BASE), total_bytes);

    // --- Audio init---
    auto &gic = core0::platform::GicCore::GetInstance();
    gic.Init({.base_addr = core0::kGicBaseAddr, .target_cpu = core0::platform::GicCore::CpuTarget::Cpu0});

    core0::platform::GpioPl::Config cfg{core0::kPlGpioBaseAddr, XPS_FPGA5_INT_ID,
                                        core0::platform::GpioPl::ChType::kChBtn,
                                        core0::platform::GpioPl::ChType::kChLed};
    core0::platform::GpioPl         gpio_pl(cfg);
    core0::platform::BoardIo        board_io;

    board_io.Init(&gpio_pl, 0x00000003, 0x0000000F);

    auto &i2s_rx = core0::platform::I2sRx::GetInstance();
    auto &i2s_tx = core0::platform::I2sTx::GetInstance();
    auto &codec  = core0::module::CodecProvider::GetInstance().Get();

    core0::app::AudioEngine sys(codec, i2s_tx, i2s_rx);

    if (i2s_rx.Init({core0::kI2sRxBaseAddr, core0::I2sMuxBaseAddr, core0::kI2sRxIrqId}) != XST_SUCCESS) {
        LOGE("I2s RX Initialization Failed");
        return -1;
    }

    if (i2s_tx.Init({core0::kI2sTxBaseAddr, core0::I2sMuxBaseAddr, core0::kI2sTxIrqId}) != XST_SUCCESS) {
        LOGE("I2s TX Initialization Failed");
        return -1;
    }

    if (!sys.Init()) {
        LOGE("System Initialization Failed");
        return -1;
    }

    // デバッグ用
    //board_io.SetButtonCallback([&](uint32_t pin, bool level) {
    //    if (pin == 0 && level) {
    //        g_do_play = true;
    //    }
    //    if (pin == 1 && level) {
    //        g_do_rec = true;
    //    }
    //});
    //board_io.EnableButtonIrq();

    core0::app::AudioIpcController app(server, sys);
    app.SetHandlers();

    app.Run();

    return 0;
}
