/**
 * @file audio_dsp_config_handler.cpp
 * @brief AudioDspConfigHandlerの実装
 */

// 自ヘッダー
#include "audio_dsp_config_handler.h"

// プロジェクトライブラリ
#include "arec_core.h"
#include "acu_core.h"
#include "logger_core.h"
#include "mapper/agc_config_mapper.h"

namespace core0 {
namespace app {

/**
 * @brief DSP設定系IPCハンドラをAppServerへ登録する
 */
void AudioDspConfigHandler::BindHandlers()
{
    server_.on_set_agc        = [&](const core::ipc::SetAgcPayload &p) -> int32_t { return OnSetAgc(p); };
    server_.on_set_rec_option = [&](const core::ipc::RecOptionPayload &p) -> int32_t { return OnSetRecOption(p); };
}

/**
 * @brief AGC設定要求を適用する
 *
 * @param[in] p AGC設定ペイロード
 * @retval 0 適用完了
 */
int32_t AudioDspConfigHandler::OnSetAgc(const core::ipc::SetAgcPayload &p)
{
    auto cfg = ToAgcConfig(p);
    platform::Agc::GetInstance().ApplyConfig(cfg);

    LOGI("SetAgc applied: en=%u dist=%d min=%d max=%d k=%d", (unsigned)p.enable, (int)p.dist_mm, (int)p.min_gain_x100,
         (int)p.max_gain_x100, (int)p.speed_k);
    return 0;
}

/**
 * @brief 録音オプション設定要求を適用する
 *
 * @param[in] p 録音オプション設定ペイロード
 * @retval 0 適用完了
 */
int32_t AudioDspConfigHandler::OnSetRecOption(const core::ipc::RecOptionPayload &p)
{
    platform::Acu::DcCutConfig dc{};
    dc.fs_hz   = 48000.0f;
    dc.fc_hz   = static_cast<float>(p.dc_fc_q16) / 65536.0f;
    dc.dc_pass = p.dc_enable ? 0u : 1u;

    platform::Acu::NoiseGateConfig ng{};
    ng.th_open   = static_cast<float>(p.ng_th_open_q15) / 32768.0f;
    ng.th_close  = static_cast<float>(p.ng_th_close_q15) / 32768.0f;
    ng.attack_s  = static_cast<float>(p.ng_attack_ms) / 1000.0f;
    ng.release_s = static_cast<float>(p.ng_release_ms) / 1000.0f;
    ng.ng_pass   = p.ng_enable ? 0u : 1u;

    auto &acu = platform::Acu::GetInstance();
    acu.SetDcCut(dc);
    acu.SetNoiseGate(ng);

    platform::Arec::Config arec{};
    arec.threshold        = p.arec_threshold;
    arec.window_shift     = p.arec_window_shift;
    arec.pretrig_samples  = p.arec_pretrig_samples;
    arec.required_windows = p.arec_required_windows;
    arec.enable           = p.arec_enable != 0u;
    platform::Arec::GetInstance().ApplyConfig(arec);

    LOGI("SetRecOption applied: dc_en=%u fc_q16=%ld ng_en=%u open_q15=%ld close_q15=%ld atk=%u rel=%u arec_en=%u arec_th=%u arec_win=%u arec_pre=%u arec_req=%u",
         (unsigned)p.dc_enable, (long)p.dc_fc_q16, (unsigned)p.ng_enable, (long)p.ng_th_open_q15,
         (long)p.ng_th_close_q15, (unsigned)p.ng_attack_ms, (unsigned)p.ng_release_ms, (unsigned)p.arec_enable,
         (unsigned)p.arec_threshold, (unsigned)p.arec_window_shift, (unsigned)p.arec_pretrig_samples,
         (unsigned)p.arec_required_windows);
    return 0;
}

}  // namespace app
}  // namespace core0
