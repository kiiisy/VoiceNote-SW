#include <cinttypes>

#define ILI9341_USE 0
#define ST7789_USE  1

#include "amp_hw.h"
#include "app_client.h"
#include "common.h"
#include "display_spec.h"
#include "gic_core.h"
#include "gpio_pl.h"
#include "gpio_ps.h"
#include "i2c_pl_core.h"
#include "lcd_bus.h"
#include "logger_core.h"
#include "lvgl_controller.h"
#include "spi_ps.h"
#include "system.h"
#include "touch_ctrl.h"

#if ILI9341_USE
#include "ili9341_panel.h"
#elif ST7789_USE
#include "st7789_panel.h"
#endif

int main()
{
    LOG_SCOPE();

    // --------------------------------------------
    // CPUブート & IPCクライアント起動
    // --------------------------------------------
    core::ipc::InitAmpOcmSharedSafe();

    auto *shared = core::ipc::GetShared();

    core1::ipc::IpcClient ipc(shared);

    // --------------------------------------------
    // 各初期化 & 処理実行
    // --------------------------------------------
    auto &gic = core1::platform::GicCore::GetInstance();

    const int gic_init_status =
        gic.Init({.base_addr = core1::common::kGicBaseAddr, .target_cpu = core1::platform::GicCore::CpuTarget::Cpu1});
    if (gic_init_status != XST_SUCCESS) {
        LOGE("Gic init failed");
        return -1;
    }

    core1::platform::SpiPs spi(
        {core1::common::kSpiPsBaseAddr, XSPIPS_CLK_PRESCALE_8, 0, XSPIPS_MASTER_OPTION | XSPIPS_FORCE_SSELECT_OPTION});
    spi.Init();

    auto &gpio_ps = core1::platform::GpioPs::GetInstance();
    gpio_ps.Configure(core1::common::kGpioPsBaseAddr);
    gpio_ps.Init();

    // このピン番号を直書きしたくない
    const uint32_t DC_PIN = 0, RESET_PIN = 9;
    gpio_ps.SetOutput(DC_PIN);
    gpio_ps.SetOutput(RESET_PIN);

    core1::platform::GpioPl::Config plcfg{core1::common::kGpioPlBaseAddr, core1::common::kGpioPlIrqId,
                                          core1::platform::GpioPl::ChType::kChIn,
                                          core1::platform::GpioPl::ChType::kChOut};

    core1::platform::GpioPl gpio_pl(plcfg);
    gpio_pl.Init();

    core1::platform::LcdBus lcd_bus(spi, gpio_ps, DC_PIN, RESET_PIN, core1::platform::LcdBus::ResetPolarity::ActiveLow);

#if ILI9341_USE
    core1::platform::Ili9341Panel panel;
#elif ST7789_USE
    core1::platform::St7789Panel panel;
#endif
    panel.Reset(lcd_bus);
    panel.Init(lcd_bus);
    panel.SetRotation(lcd_bus, core1::platform::PanelInterface::MADCTL_MX | core1::platform::PanelInterface::MADCTL_MV |
                                   core1::platform::PanelInterface::MADCTL_ML |
                                   core1::platform::PanelInterface::MADCTL_BGR);

    // --------------------------------------------
    // GUI生成（LVGL） & タッチ起動
    // --------------------------------------------
    core1::gui::LvglController lvgl({.hor              = core1::common::display::kWidth,
                                     .ver              = core1::common::display::kHeight,
                                     .swapRgb565Bytes  = true,
                                     .flushChunkPixels = 512});
    lvgl.Init(lcd_bus, panel);
    lvgl.SetRotation(LV_DISP_ROTATION_0);

    auto &i2c = core1::platform::I2cPlCore::GetInstance();

    const int i2c_init_status =
        i2c.Init({.base_addr = core1::common::kI2cPlBaseAddr, .irq_id = core1::common::kI2cPlIrqId});
    if (i2c_init_status != XST_SUCCESS) {
        LOGE("I2C init failed");
        return -1;
    }

    core1::platform::TouchCtrl::Config tcfg{.addr     = core1::platform::kTouchI2cAddr7,
                                            .width    = core1::common::display::kWidth,
                                            .height   = core1::common::display::kHeight,
                                            .swap_xy  = true,
                                            .invert_x = false,
                                            .invert_y = true};
    static core1::platform::TouchCtrl  touch(tcfg);

    const bool touch_init_ok = touch.Init(i2c);
    if (!touch_init_ok) {
        LOGE("Touch init failed");
        return -1;
    }

    gpio_pl.SetIrqCallback([&]() {
        gpio_pl.DisableIrq();
        touch.NotifyInterrupt();
    });
    gpio_pl.EnableIrq();

    lvgl.RegisterTouch(touch);

    lvgl.InitScreens();

    // --------------------------------------------
    // システム起動
    // --------------------------------------------
    core1::app::System sys;
    sys.Init({.ipc = &ipc, .gui = &lvgl, .touch = &touch, .gpio_pl = &gpio_pl});

    sys.Run();
}
