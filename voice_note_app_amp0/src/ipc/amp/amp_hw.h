/**
 * @file amp_hw.h
 * @brief AMP（Cortex-A9デュアルコア）向け低レベル初期化API
 */
#pragma once

#include <cstdint>

namespace core {
namespace ipc {

void InitAmpOcmSharedSafe();
void EnableGicDistributorGroup01();

}  // namespace ipc
}  // namespace core
