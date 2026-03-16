/**
 * @file audio_dsp_config_handler.h
 * @brief DSP設定系IPCハンドラ
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "app_server.h"

namespace core0 {
namespace app {

/**
 * @brief AGC/ACU設定要求を処理する
 */
class AudioDspConfigHandler final
{
public:
    explicit AudioDspConfigHandler(ipc::AppServer &server) : server_(server) {}

    void BindHandlers();

private:
    int32_t OnSetAgc(const core::ipc::SetAgcPayload &p);
    int32_t OnSetRecOption(const core::ipc::RecOptionPayload &p);

    ipc::AppServer &server_;
};

}  // namespace app
}  // namespace core0
