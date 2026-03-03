/**
 * @file ssm2603.h
 * @brief SSM2603 オーディオコーデック実装（I2C 制御）
 *
 * AudioCodec インターフェースの具体実装として、SSM2603 を I2C 経由で制御する。
 * - 初期化シーケンス（レジスタ設定 + 待ち時間）
 * - 入出力音量設定（0..100%）
 * - ミュート制御（外部 GPIO を利用したミュート線制御）
 *
 * 注意:
 * - SSM2603 のレジスタ書き込みは 9bit フォーマット（レジスタ+上位1bit）を扱う必要がある。
 * - i2c_.Init() はコンストラクタで実行される。
 */
#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "addr_map.h"
#include "codec_if.h"
#include "i2c_pl_core.h"
#include "irq_map.h"

namespace core0 {
namespace module {

#define SSM2603_I2C_ADDR 0x1A  // CFGピンで 0x1B の場合あり

/**
 * @class Ssm2603
 * @brief SSM2603 コーデック制御クラス（Singleton）
 *
 * - AudioCodec の実装として Init/音量/Mute を提供する
 * - I2C アクセスは I2cPlCore を利用する
 */
class Ssm2603 : public AudioCodec
{
public:
    static Ssm2603 &GetInstance()
    {
        static Ssm2603 inst;
        return inst;
    }

    bool Init();
    bool SetOutVolume(uint8_t Lpct, uint8_t Rpct);
    bool SetInVolume(uint8_t Lpct, uint8_t Rpct);
    void Mute(bool en) { i2c_.GpioOutput(en ? 0x00 : 0xFF); }

private:
    /**
     * @brief SSM2603 レジスタアドレス
     */
    enum Reg : uint8_t
    {
        LEFT_LINE_IN  = 0x00,
        RIGHT_LINE_IN = 0x01,
        LEFT_HP_OUT   = 0x02,
        RIGHT_HP_OUT  = 0x03,
        ANALOG_PATH   = 0x04,
        DIGITAL_PATH  = 0x05,
        POWER_DOWN    = 0x06,
        DIGITAL_IF    = 0x07,
        SAMPLING      = 0x08,
        ACTIVE_CTRL   = 0x09,
        RESET         = 0x0F,
    };

    /**
     * @brief I2C スレーブアドレスを設定する
     * @retval true  成功
     * @retval false 失敗
     */
    bool SetSlaveAddr() { return i2c_.SetSlaveAddr(SSM2603_I2C_ADDR); }

    /**
     * @brief レジスタへ書き込む（SSM2603 の 9bit フォーマット）
     *
     * 9bit データ（D8..D0）を、以下の 2 バイトへパックして送信する:
     * - b0: [reg(7bit) << 1] | D8
     * - b1: D7..D0
     *
     * @param[in] reg  レジスタ
     * @param[in] data 書き込みデータ（下位 9bit を使用）
     * @retval true  成功
     * @retval false 失敗
     */
    bool Write(Reg reg, uint16_t data)
    {
        // 9bit 形式のため、上位1bitをレジスタアドレスに含める
        uint8_t b0     = (static_cast<uint8_t>(reg) << 1) | ((data >> 8) & 0x1);
        uint8_t b1     = static_cast<uint8_t>(data & 0xFF);
        uint8_t buf[2] = {b0, b1};

        return i2c_.Write(buf, 2);
    }

    /**
     * @brief レジスタを読み出す（9bit 復元）
     *
     * 実装の前提:
     * - subaddress で reg を指定（D8=0 で指定）
     * - 2 バイト読み、9bit を復元する
     *
     * @param[in]  reg  読み出し対象レジスタ
     * @param[out] out  復元した 9bit 値（nullptr の場合は失敗）
     * @retval true  成功
     * @retval false 失敗
     */
    bool Read(Reg reg, uint16_t *out)
    {
        if (!out) {
            return false;
        }

        const uint8_t sub   = static_cast<uint8_t>(reg) << 1;  // D8=0で指定
        uint8_t       rx[2] = {0, 0};
        if (!i2c_.ReadSubAddr(sub, rx, 2)) {
            return false;
        }
        // 復元：rx[0]=D7..D0、rx[1].bit0=D8
        *out = static_cast<uint16_t>((rx[1] & 0x01) << 8) | rx[0];

        return true;
    }

    /**
     * @brief 0..100(%) を 0..0x7F へ線形変換する
     *
     * @param[in] pct 0..100
     * @return 0..0x7F の音量コード
     */
    static uint8_t VolPctToCode(uint8_t pct) { return (pct > 100) ? 0x7F : (uint8_t)(pct * 0x7F / 100); }

    Ssm2603() : i2c_(platform::I2cPlCore::GetInstance())
    {
        (void)i2c_.Init({.base_addr = core0::kI2cPlBaseAddr, .irq_id = core0::kI2cPlIrqId});
    }
    ~Ssm2603() {};
    bool SetSampleRate(uint32_t fs_hz);

    platform::I2cPlCore &i2c_;  // I2C制御コア
    uint8_t              addr_;
};
}  // namespace module
}  // namespace core0
