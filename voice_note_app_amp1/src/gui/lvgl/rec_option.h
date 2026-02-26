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
};

}  // namespace gui
}  // namespace core1
