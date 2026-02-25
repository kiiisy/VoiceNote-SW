#pragma once
#include "gic_core.h"
#include "gpio_if.h"
#include "xgpio.h"
#include "xiltimer.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xstatus.h"
#include <cstdint>

namespace core0 {
namespace platform {

// AXI GPIO (XGpio) 実装。
// - ch_btn: 入力チャネル（ボタン群）を 0..31 の論理pin として公開
// - ch_led: 出力チャネル（LED群）を 0..31 の論理pin として公開
class GpioPl final : public GpioIf
{
public:
    enum ChType : uint32_t
    {
        kChBtn = 0x01,
        kChLed,
    };

    struct Config
    {
        uint32_t addr;
        uint32_t irq_id;
        uint32_t ch_btn;
        uint32_t ch_led;
    };

    explicit GpioPl(const Config &c) : cfg_(c) {}

    bool Init() override;
    void Deinit() override;

    void     SetDir(uint32_t pin, GpioDir d) override;     // LED 側のbit方向のみ Out を想定
    uint32_t ReadPin(uint32_t pin) override;               // BTN 側のbitのみ
    void     WritePin(uint32_t pin, uint32_t v) override;  // LED 側のみ

    uint32_t ReadInputs() override;                 // BTN チャネル全体
    void     WriteOutputs(uint32_t mask) override;  // LED チャネル全体

    bool EnableIrq() override;  // pinは未使用（チャネル単位で有効化）
    void DisableIrq() override;
    void SetIrqCallback(GpioIrqCallback cb) override { cb_ = std::move(cb); }

    void PollDebounce();

private:
    const uint32_t in_setting_  = 0xFFFFFFFF;
    const uint32_t out_setting_ = 0x00000000;

    static void IsrThunk(void *ref);
    void        OnIsr();
    uint64_t    NowUsec();
    enum class DbState
    {
        Idle,
        Waiting,
        Ready
    };
    volatile DbState          db_state_       = DbState::Idle;
    volatile bool             need_debounce_  = false;
    static constexpr uint32_t DEBOUNCE_MS     = 15;
    volatile uint64_t         db_deadline_us_ = 0;

    XGpio           inst_{};
    Config          cfg_{};
    bool            irq_attached_ = false;
    GpioIrqCallback cb_{};
    uint32_t        last_btn_ = 0;
};
}  // namespace platform
}  // namespace core0
