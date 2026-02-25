#include "board_io.h"
#include "logger_core.h"

namespace core0 {
namespace platform {

bool BoardIo::Init(GpioIf *port, uint32_t btn_bits_mask, uint32_t led_bits_mask)
{
    LOG_SCOPE();

    port_     = port;
    btn_mask_ = btn_bits_mask;
    led_mask_ = led_bits_mask;
    btn_last_ = 0;
    if (!port_ || !port_->Init()) {
        LOGE("Port Initialize Failed");
        return false;
    }

    // LED: 出力化
    for (uint32_t bit = 0; bit < 32; ++bit) {
        if (led_mask_ & (1u << bit)) {
            port_->SetDir(bit, GpioDir::Out);
        }
    }
    // BTN: 入力化
    for (uint32_t bit = 0; bit < 32; ++bit) {
        if (btn_mask_ & (1u << bit)) {
            port_->SetDir(bit, GpioDir::In);
        }
    }

    // 初期LEDは消灯
    WriteLeds(0);
    // 初回ボタン状態
    btn_last_ = port_->ReadInputs() & btn_mask_;

    return true;
}

void BoardIo::Deinit()
{
    if (!port_) {
        LOGW("Port is not activate");
        return;
    }

    DisableButtonIrq();
    port_->Deinit();
    port_ = nullptr;
}

void BoardIo::SetLed(uint32_t index, bool on)
{
    if (!port_) {
        LOGW("Port is not activate");
        return;
    }

    uint32_t bit = (1u << index);
    if (!(led_mask_ & bit)) {
        return;
    }

    // 現在値を読んで 1bit だけ上書き
    uint32_t cur = 0;  // 実装依存：WriteOutputs しかない場合は ReadPin で組み立て
    for (uint32_t i = 0; i < 32; ++i) {
        if (led_mask_ & (1u << i)) {
            cur |= (port_->ReadPin(i) ? (1u << i) : 0);
        }
    }

    if (on) {
        cur |= bit;
    } else {
        cur &= ~bit;
    }

    port_->WriteOutputs(cur);
}

void BoardIo::ToggleLed(uint32_t index)
{
    if (!port_) {
        LOGW("Port is not activate");
        return;
    }

    uint32_t bit = (1u << index);
    if (!(led_mask_ & bit)) {
        return;
    }

    led_last_ ^= bit;
    port_->WriteOutputs(led_last_);
}

void BoardIo::WriteLeds(uint32_t mask)
{
    if (!port_) {
        LOGW("Port is not activate");
        return;
    }

    // mask の中で led_mask_ に該当するビットのみ反映
    uint32_t out = 0;
    for (uint32_t i = 0; i < 32; ++i) {
        if (led_mask_ & (1u << i)) {
            out |= (mask & (1u << i));
        }
    }

    port_->WriteOutputs(out);
}

bool BoardIo::ReadButtons(uint32_t *out_state, uint32_t *out_changed)
{
    if (!port_) {
        LOGW("Port is not activate");
        return false;
    }

    uint32_t now = port_->ReadInputs() & btn_mask_;
    uint32_t chg = now ^ btn_last_;
    if (out_state) {
        *out_state = now;
    }

    if (out_changed) {
        *out_changed = chg;
    }

    btn_last_ = now;
    return true;
}

void BoardIo::SetButtonCallback(std::function<void(uint32_t pin, bool level)> cb)
{
    if (!port_) {
        LOGW("Port is not activate");
        return;
    }

    port_->SetIrqCallback([this, cb](uint32_t pin, bool level) {
        if ((btn_mask_ & (1u << pin)) && cb) {
            cb(pin, level);
        } else {
            LOGE("Interrupt Handler Failed to register");
            return;
        }
    });
}

bool BoardIo::EnableButtonIrq()
{
    if (!port_) {
        LOGW("Port is not activate");
        return false;
    }

    port_->EnableIrq();

    return true;
}

void BoardIo::DisableButtonIrq()
{
    if (!port_) {
        LOGW("Port is not activate");
        return;
    }

    port_->DisableIrq();
}
}  // namespace platform
}  // namespace core0
