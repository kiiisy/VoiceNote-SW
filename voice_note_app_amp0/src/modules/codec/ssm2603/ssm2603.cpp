/**
 * @file codec_ssm2603.cpp
 * @brief SSM2603 オーディオコーデック実装および既定コーデック解決
 *
 * - CodecProvider::EnsureDefault(): 既定の AudioCodec 実装として Ssm2603 を返す
 * - Ssm2603: I2C 経由で SSM2603 を初期化/制御する実装
 *
 * 注意:
 * - SSM2603 の初期化シーケンスは、リセット後の安定化待ちや VMID 充電待ちを含む。
 * - 本実装は Baremetal 想定で usleep() による待ち時間を使用する。
 */

// 自ヘッダー
#include "ssm2603.h"

// Vitisライブラリ
#include <sleep.h>

// プロジェクトライブラリ
#include "codec_provider.h"
#include "logger_core.h"

namespace core0 {
namespace module {

/**
 * @brief 既定の AudioCodec 実装を返す
 *
 * CodecProvider に実装が設定されていない場合に呼ばれ、
 * 既定実装として Ssm2603 の Singleton を採用する。
 *
 * @return 現在有効な AudioCodec 実装ポインタ（nullptr にならない想定）
 */
AudioCodec *CodecProvider::EnsureDefault()
{
    if (!impl_) {
        impl_ = &Ssm2603::GetInstance();
    }

    return impl_;
}

/**
 * @brief SSM2603 を初期化する
 *
 * 代表的な初期化フロー:
 * - ミュートを有効化（不要なポップノイズ抑制）
 * - I2C Slave Address 設定
 * - ソフトリセット
 * - Power/Analog/Digital/DigitalIF/Sampling の初期設定
 * - VMID 充電待ち（データシート推奨の遅延）
 * - Active 化
 * - 出力を有効化（PowerDown解除）
 * - ミュート解除
 *
 * @retval true  初期化成功
 * @retval false 初期化失敗（I2C 通信失敗など）
 */
bool Ssm2603::Init()
{
    LOG_SCOPE();

    Mute(true);  // ミュート

    if (!SetSlaveAddr()) {
        LOGE("The slave address is invalid");
        return false;
    }

    // ソフトリセット
    if (!Write(RESET, 0x000)) {
        return false;
    }

    // 10ms 待機（リセット後の安定化のため）
    usleep(10000);

    // 1. Power Managementの設定（出力はまだ設定しない）
    if (!Write(POWER_DOWN, 0x1EF)) {
        LOGW("Failed to write POWER_DOWN");
    }

    // 2. Left and Right Line Inputの音量調整
    if (!Write(LEFT_LINE_IN, 0x017)) {
        LOGW("Failed to write LEFT_LINE_IN");
    }

    if (!Write(RIGHT_LINE_IN, 0x017)) {
        LOGW("Failed to write RIGHT_LINE_IN");
    }

    // 3. Left and Right Headphone Outputの音量調整
    if (!Write(LEFT_HP_OUT, 0x179)) {
        LOGW("Failed to write LEFT_HP_OUT");
    }

    //if (!Write(RIGHT_HP_OUT, 0x17F)) {
    if (!Write(RIGHT_HP_OUT, 0x179)) {
        LOGW("Failed to write RIGHT_HP_OUT");
    }

    // 4. Analog Pathの設定
    if (!Write(ANALOG_PATH, 0x015)) {
        LOGW("Failed to write ANALOG_PATH");
    }

    // 5. Digital Pathの設定
    if (!Write(DIGITAL_PATH, 0x000)) {
        LOGW("Failed to write DIGITAL_PATH");
    }

    // 6. Digital Interfaceの設定
    if (!Write(DIGITAL_IF, 0x002)) {
        LOGW("Failed to write DIGITAL_IF");
    }

    // 7. Sample Rateの設定
    if (!SetSampleRate(48000)) {
        LOGW("Failed to set sample rate");
    }

    // VMIDのキャパシタの充電のために十分な遅延時間を挿入（データシートより50ms以上推奨）
    usleep(50000);

    // 8. Active Controlの設定
    if (!Write(ACTIVE_CTRL, 0x001)) {
        LOGW("Failed to write ACTIVE_CTRL");
    }

    // 9. Power Managementの設定（出力を有効化）
    if (!Write(POWER_DOWN, 0x000)) {
        LOGW("Failed to write POWER_DOWN");
    }

    // 起動時のreadbackは環境によって失敗しやすく、不要なrecoverを誘発するため実施しない。
    // 10. muteを解除
    Mute(false);

    return true;
}

/**
 * @brief サンプリングレートを設定する
 *
 * SSM2603 の Sampling Control レジスタへ設定値を書き込む。
 * fs_hz は代表値のみを扱い、それ以外は fallback として 48kHz 相当を設定する。
 *
 * @param[in] fs_hz サンプリング周波数 [Hz]（例: 48000）
 * @retval true  設定成功
 * @retval false 設定失敗（I2C 書き込み失敗）
 */
bool Ssm2603::SetSampleRate(uint32_t fs_hz)
{
    uint16_t v = 0;  // 実機の MCLK 構成に合わせて調整
    switch (fs_hz) {
        case 8000:
            v = 0b0000'0110;
            break;
        case 16000:
            v = 0b0000'1100;
            break;
        case 32000:
            v = 0b0000'0011;
            break;
        case 44100:
            v = 0b0000'1000;
            break;  // 近似例
        case 48000:
            v = 0b0000'0000;
            break;
        case 96000:
            v = 0b0001'0000;
            break;
        default:
            v = 0b0000'0000;
            break;  // fallback 48k
    }
    return Write(SAMPLING, v);
}

/**
 * @brief ヘッドホン出力音量を設定する
 *
 * 0..100(%) をコーデックの音量コードへ線形マップし、左右独立に設定する。
 * UPDATE ビット（bit8）を立て、同時更新を意図する。
 *
 * @param[in] Lpct 左出力音量（0..100）
 * @param[in] Rpct 右出力音量（0..100）
 * @retval true  設定成功
 * @retval false 設定失敗（I2C 書き込み失敗）
 */
bool Ssm2603::SetOutVolume(uint8_t Lpct, uint8_t Rpct)
{
    uint16_t L = VolPctToCode(Lpct) | (1u << 8);  // UPDATE
    uint16_t R = VolPctToCode(Rpct) | (1u << 8);
    if (!Write(LEFT_HP_OUT, L)) {
        return false;
    }
    if (!Write(RIGHT_HP_OUT, R)) {
        return false;
    }
    return true;
}

/**
 * @brief ライン入力音量（入力ゲイン）を設定する
 *
 * 0..100(%) をコーデックの音量コードへ線形マップし、左右独立に設定する。
 * UPDATE ビット（bit8）を立て、同時更新を意図する。
 *
 * @param[in] Lpct 左入力音量（0..100）
 * @param[in] Rpct 右入力音量（0..100）
 * @retval true  設定成功
 * @retval false 設定失敗（I2C 書き込み失敗）
 */
bool Ssm2603::SetInVolume(uint8_t Lpct, uint8_t Rpct)
{
    uint16_t L = VolPctToCode(Lpct) | (1u << 8);  // UPDATE
    uint16_t R = VolPctToCode(Rpct) | (1u << 8);
    if (!Write(LEFT_LINE_IN, L)) {
        return false;
    }
    if (!Write(RIGHT_LINE_IN, R)) {
        return false;
    }
    return true;
}
}  // namespace module
}  // namespace core0
