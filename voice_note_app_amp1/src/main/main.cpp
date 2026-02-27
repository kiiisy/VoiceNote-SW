#include "amp_hw.h"
#include "app_client.h"
#include "common.h"
#include "display/display_spec.h"
#include "gic_core.h"
#include "gpio_pl.h"
#include "gpio_ps.h"
#include "i2c_pl_core.h"
#include "ili9341_panel.h"
#include "lcd_bus.h"
#include "logger_core.h"
#include "lvgl_controller.h"
#include "sleep.h"
#include "spi_ps.h"
#include "st7789_panel.h"
#include "system.h"
#include "touch_ctrl.h"
#include "xparameters.h"
#include <cinttypes>

#define ILI9341_USE 0
#define ST7789_USE  1

int main()
{
    LOG_SCOPE();

    // -------------------------
    // AMP設定
    // -------------------------
    core::ipc::InitAmpOcmSharedSafe();
    auto *shared = core::ipc::GetShared();

    core1::ipc::IpcClient ipc(shared);

    // デバッグ用コールバック
    ipc.on_ack = [](core::ipc::CmdId cmd, uint32_t seq, core::ipc::AckStatus st, int32_t rc) {
        LOGD("ack cmd=%u st=%u seq=%u rc=%" PRId32 "\r\n", (unsigned)cmd, (unsigned)st, (unsigned)seq, rc);
    };
    ipc.on_playback_status = [](const core::ipc::PlaybackStatusPayload &st) {
        LOGD("pb pos=%u dur=%u rem=%u state=%u\r\n", (unsigned)st.position_ms, (unsigned)st.duration_ms,
             (unsigned)st.remain_ms, (unsigned)st.state);
    };

    // -------------------------
    // 割り込みコントローラーの初期化
    // -------------------------
    auto &gic = core1::platform::GicCore::GetInstance();
    if (gic.Init({.base_addr = XPAR_XSCUGIC_0_BASEADDR, .target_cpu = core1::platform::GicCore::CpuTarget::Cpu1}) !=
        XST_SUCCESS) {
        LOGE("Gic init failed");
        return -1;
    }

    core1::platform::SpiPs spi(
        {XPAR_XSPIPS_0_BASEADDR, XSPIPS_CLK_PRESCALE_8, 0, XSPIPS_MASTER_OPTION | XSPIPS_FORCE_SSELECT_OPTION});
    spi.Init();

    auto &gpio_ps = core1::platform::GpioPs::GetInstance();
    gpio_ps.Configure(XPAR_XGPIOPS_0_BASEADDR);
    gpio_ps.Init();

    const uint32_t DC_PIN = 0, RESET_PIN = 9;
    gpio_ps.SetOutput(DC_PIN);
    gpio_ps.SetOutput(RESET_PIN);

    core1::platform::GpioPl::Config plcfg{XPAR_XGPIO_1_BASEADDR, XPS_FPGA7_INT_ID,
                                          core1::platform::GpioPl::ChType::kChIn,
                                          core1::platform::GpioPl::ChType::kChOut};
    core1::platform::GpioPl         gpio_pl(plcfg);
    gpio_pl.Init();

    core1::platform::LcdBus lcd_bus(spi, gpio_ps, DC_PIN, RESET_PIN, core1::platform::LcdBus::ResetPolarity::ActiveLow);

#if ILI9341_USE
    Ili9341Panel panel;
    panel.Reset(lcd_bus);
    panel.Init(lcd_bus);
    panel.SetRotation(lcd_bus, PanelInterface::MADCTL_MY | PanelInterface::MADCTL_MV);
#elif ST7789_USE
    core1::platform::St7789Panel panel;
    panel.Reset(lcd_bus);
    panel.Init(lcd_bus);
    panel.SetRotation(lcd_bus, core1::platform::PanelInterface::MADCTL_MX | core1::platform::PanelInterface::MADCTL_MV |
                                   core1::platform::PanelInterface::MADCTL_ML |
                                   core1::platform::PanelInterface::MADCTL_BGR);
#endif

    // -------------------------
    // LVGL + Touch
    // -------------------------
    core1::gui::LvglController lvgl({.hor              = core1::common::display::kWidth,
                                     .ver              = core1::common::display::kHeight,
                                     .swapRgb565Bytes  = true,
                                     .flushChunkPixels = 512});
    lvgl.Init(lcd_bus, panel);
    lvgl.SetRotation(LV_DISP_ROTATION_0);

    auto &i2c = core1::platform::I2cPlCore::GetInstance();
    if (i2c.Init({.base_addr = core1::common::kI2cPlBaseAddr, .irq_id = core1::common::kI2cPlIrqId}) != XST_SUCCESS) {
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
    if (!touch.Init(i2c)) {
        LOGE("Touch init failed");
        return -1;
    }

    // タッチ割り込み → NotifyInterrupt
    gpio_pl.SetIrqCallback([&]() {
        gpio_pl.DisableIrq();
        touch.NotifyInterrupt();
    });
    gpio_pl.EnableIrq();

    lvgl.RegisterTouch(touch);

    // GUI生成
    lvgl.InitScreens();

    // -------------------------
    // System
    // -------------------------
    core1::app::System sys;
    sys.Init({.ipc = &ipc, .gui = &lvgl, .touch = &touch, .gpio_pl = &gpio_pl});

    // -------------------------
    // メイン処理
    // -------------------------
    sys.Run();
}
