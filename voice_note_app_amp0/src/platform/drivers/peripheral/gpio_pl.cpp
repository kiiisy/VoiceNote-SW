#include "gpio_pl.h"
#include "gic_core.h"
#include "logger_core.h"

namespace core0 {
namespace platform {

bool GpioPl::Init()
{
    LOG_SCOPE();

    auto *cfg = XGpio_LookupConfig(cfg_.addr);
    if (!cfg) {
        LOGE("PL Gpio LookupConfig Failed");
        return false;
    }

    if (XGpio_CfgInitialize(&inst_, cfg, cfg->BaseAddress) != XST_SUCCESS) {
        LOGE("PL Gpio CfgInitialize Failed");
        return false;
    }

    // 方向設定
    XGpio_SetDataDirection(&inst_, cfg_.ch_btn, in_setting_);   // 入力
    XGpio_SetDataDirection(&inst_, cfg_.ch_led, out_setting_);  // 出力
    return true;
}

void GpioPl::Deinit()
{
    if (irq_attached_) {
        auto &gic = GicCore::GetInstance();
        gic.Disable(cfg_.irq_id);
        gic.Detach(cfg_.irq_id);
        irq_attached_ = false;
    }
}

void GpioPl::SetDir(uint32_t pin, GpioDir d)
{
    // LED チャネル全体の方向のみ Out を維持
    (void)pin;
    (void)d;
}

uint32_t GpioPl::ReadPin(uint32_t pin)
{
    uint32_t v = XGpio_DiscreteRead(&inst_, cfg_.ch_btn);
    return (v >> pin) & 1u;
}

void GpioPl::WritePin(uint32_t pin, uint32_t val)
{
    uint32_t cur = XGpio_DiscreteRead(&inst_, cfg_.ch_led);
    if (val) {
        cur |= (1u << pin);
    } else {
        cur &= ~(1u << pin);
    }

    XGpio_DiscreteWrite(&inst_, cfg_.ch_led, cur);
}

uint32_t GpioPl::ReadInputs() { return XGpio_DiscreteRead(&inst_, cfg_.ch_btn); }

void GpioPl::WriteOutputs(uint32_t mask) { XGpio_DiscreteWrite(&inst_, cfg_.ch_led, mask); }

bool GpioPl::EnableIrq()
{
    // ボタン（インプット）の割り込みのみ有効化
    if (!irq_attached_) {
        auto &gic = GicCore::GetInstance();
        if (gic.Attach(cfg_.irq_id, (Xil_InterruptHandler)&GpioPl::IsrThunk, this, GicCore::GicPriority::Normal,
                       GicCore::GicTrigger::LevelHigh) == XST_SUCCESS) {

            gic.Enable(cfg_.irq_id);
            irq_attached_ = true;
        } else {
            LOGE("Attaching an interrupt signal failed");
            return false;
        }
    }

    XGpio_InterruptEnable(&inst_, cfg_.ch_btn);
    XGpio_InterruptGlobalEnable(&inst_);

    return true;
}

void GpioPl::DisableIrq()
{
    XGpio_InterruptDisable(&inst_, cfg_.ch_btn);
    XGpio_InterruptGlobalDisable(&inst_);
}

void GpioPl::IsrThunk(void *ref) { static_cast<GpioPl *>(ref)->OnIsr(); }

void GpioPl::OnIsr()
{
    // 割り込み要因をクリア
    XGpio_InterruptClear(&inst_, cfg_.ch_btn);

    need_debounce_  = true;
    db_deadline_us_ = NowUsec() + (uint64_t)DEBOUNCE_MS * 1000ULL;
    db_state_       = DbState::Waiting;
}

void GpioPl::PollDebounce()
{
    if (db_state_ == DbState::Idle) {
        return;
    }

    if (db_state_ == DbState::Waiting) {
        LOGD("DB -> Ready (dl=%llu)", db_deadline_us_);
        if (NowUsec() < db_deadline_us_) {
            return;
        }
        db_state_ = DbState::Ready;
    }

    if (db_state_ == DbState::Ready) {
        // 現在値をスナップショット
        uint32_t s    = XGpio_DiscreteRead(&inst_, cfg_.ch_btn);
        uint32_t prev = last_btn_;
        uint32_t chg  = s ^ prev;

        LOGD("DB commit: prev=%08x now=%08x chg=%08x", prev, s, chg);

        if (cb_ && chg) {
            for (uint32_t bit = 0; bit < 4; ++bit) {
                uint32_t m = 1u << bit;
                if ((chg & m) != 0) {
                    bool raw_now  = (s >> bit) & 1u;
                    bool raw_prev = (prev >> bit) & 1u;

                    bool pressed_now  = !raw_now;  // アクティブロー補正
                    bool pressed_prev = !raw_prev;

                    LOGD("CB check: bit=%u raw_now=%u pressed_now=%u raw_prev=%u pressed_prev=%u", bit, raw_now,
                         pressed_now, raw_prev, pressed_prev);

                    if (!pressed_prev && pressed_now) {
                        cb_(bit, true);  // 押下の立ち上がりだけ
                        LOGD("CB call (press edge): bit=%u", bit);
                    }
                }
            }
        }

        // ★ここで初めて last_btn_ を更新（順序が重要）
        last_btn_ = s;

        db_state_ = DbState::Idle;
    }
}

static inline void EnableArmGlobalTimerOnce()
{
    static bool done = false;
    // 1発目だけ動かしてやる
    if (!done) {
        const u32 GT_CTRL = 0xF8F00208u;               // Cortex-A9 Global Timer Control
        Xil_Out32(GT_CTRL, Xil_In32(GT_CTRL) | 0x1u);  // bit0 = enable
        done = true;
    }
}

uint64_t GpioPl::NowUsec()
{
    EnableArmGlobalTimerOnce();
    XTime time;

    XTime_GetTime(&time);

    const uint64_t GT_HZ = (XPAR_CPU_CORE_CLOCK_FREQ_HZ / 2);

    return (uint64_t)((time * 1000000ULL) / GT_HZ);
}
}  // namespace platform
}  // namespace core0
