#pragma once

// 標準ライブラリ
#include <cstdint>

namespace core1 {
namespace gui {

struct RecOptionRequest
{
    bool     dc_enable{true};
    uint16_t dc_fc_hz{20};

    bool     ng_enable{false};
    uint16_t ng_th_open_x1000{50};
    uint16_t ng_th_close_x1000{40};
    uint16_t ng_attack_ms{5};
    uint16_t ng_release_ms{50};

    bool     arec_enable{false};
    uint16_t arec_threshold{0x0200};
    uint8_t  arec_window_shift{6};
    uint16_t arec_pretrig_samples{512};
    uint8_t  arec_required_windows{2};
};

}  // namespace gui
}  // namespace core1
