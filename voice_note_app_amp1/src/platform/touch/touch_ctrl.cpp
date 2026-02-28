/**
 * @file touch_ctrl.cpp
 * @brief touch_ctrlの実装
 */

// 自ヘッダー
#include "touch_ctrl.h"

// 標準ライブラリ
#include <algorithm>

// Vitisライブラリ
#include "sleep.h"

// プロジェクトライブラリ
#include "i2c_pl_core.h"
#include "logger_core.h"

namespace core1 {
namespace platform {

namespace {

// 結構無理矢理な数値入れてる
static constexpr int16_t RAW_X_MIN = 22;   // 生Xの最小キャリブ値
static constexpr int16_t RAW_X_MAX = 288;  // 生Xの最大キャリブ値
static constexpr int16_t RAW_Y_MIN = 17;   // 生Yの最小キャリブ値
static constexpr int16_t RAW_Y_MAX = 242;  // 生Yの最大キャリブ値

// 押下中ポーリング(約5ms周期)での離し確定猶予
// 6回連続で非押下を観測してから離し確定する（約30ms）
static constexpr uint8_t  kReleaseDebounceCount = 6;    // 離し確定までの連続非押下回数
static constexpr uint8_t  kTouchReadRetryCount  = 3;    // タッチ状態読み取りのリトライ回数
static constexpr uint32_t kTouchReadRetryUs     = 200;  // リトライ間隔[us]
static constexpr uint32_t kTouchErrLogInterval  = 16;   // エラーログの間引き周期

static constexpr uint16_t kRegTouchData       = 0xD000;  // 座標データ先頭レジスタ
static constexpr uint16_t kRegTouchStatus     = 0xD005;  // タッチ状態/ACKレジスタ
static constexpr uint8_t  kFrameMarkerIndex   = 6;       // フレームマーカのバイト位置
static constexpr uint8_t  kFrameMarkerValue   = 0xAB;    // 有効フレームのマーカ値
static constexpr uint8_t  kTouchDataBlockSize = 7;       // 1点分の読み取りサイズ[byte]

}  // namespace

/**
 * @brief 生座標を画面座標へ変換する
 *
 * @param[in,out] point 変換対象（x,y を更新）
 */
void TouchCtrl::MapRawToScreen(TouchPoint &point) const
{
    // 生座標を取り出す
    int16_t rx = point.x;
    int16_t ry = point.y;

    // パネル向きの設定を順に反映
    if (cfg_.swap_xy) {
        std::swap(rx, ry);
    }
    if (cfg_.invert_x) {
        rx = RAW_X_MIN + RAW_X_MAX - rx;
    }
    if (cfg_.invert_y) {
        ry = RAW_Y_MIN + RAW_Y_MAX - ry;
    }

    // 変換レンジは固定値で扱う（操作中に揺れないように）
    const int32_t xmin = RAW_X_MIN;
    const int32_t xmax = RAW_X_MAX;
    const int32_t ymin = RAW_Y_MIN;
    const int32_t ymax = RAW_Y_MAX;

    // 生レンジ幅（ゼロ割り防止のため最低1とする）
    const int32_t xr = std::max<int32_t>(1, xmax - xmin);
    const int32_t yr = std::max<int32_t>(1, ymax - ymin);

    // (raw - min) を画面サイズへ
    int32_t x = ((static_cast<int32_t>(rx) - xmin) * (static_cast<int32_t>(cfg_.width) - 1) + xr / 2) / xr;
    int32_t y = ((static_cast<int32_t>(ry) - ymin) * (static_cast<int32_t>(cfg_.height) - 1) + yr / 2) / yr;

    // 最後に画面サイズへ収める
    x = std::clamp<int32_t>(x, 0, static_cast<int32_t>(cfg_.width) - 1);
    y = std::clamp<int32_t>(y, 0, static_cast<int32_t>(cfg_.height) - 1);

    point.x = (uint16_t)x;
    point.y = (uint16_t)y;
}

/**
 * @brief 3点中央値フィルタで座標を平滑化する
 *
 * @param[in,out] point 入力/出力座標
 */
void TouchCtrl::MedianFilter3(TouchPoint *point)
{
    // 直近3点だけ持つ簡易フィルタ
    static uint16_t x_hist[3];
    static uint16_t y_hist[3];
    static uint16_t hist_index = 0;
    static bool     hist_init  = false;

    auto med3 = [](uint16_t a, uint16_t b, uint16_t c) {
        if (a > b) {
            std::swap(a, b);
        }
        if (b > c) {
            std::swap(b, c);
        }
        if (a > b) {
            std::swap(a, b);
        }
        return b;  // 中央値
    };

    if (point->pressed) {
        // 押し始めは履歴を同値で埋めて急な飛びを抑える
        if (!hist_init) {  // 最初の1回は現在値で初期化
            x_hist[0] = x_hist[1] = x_hist[2] = point->x;
            y_hist[0] = y_hist[1] = y_hist[2] = point->y;
            hist_index                        = 0;
            hist_init                         = true;
        }
        x_hist[hist_index] = point->x;
        y_hist[hist_index] = point->y;
        hist_index         = (hist_index + 1) % 3;
        point->x           = med3(x_hist[0], x_hist[1], x_hist[2]);
        point->y           = med3(y_hist[0], y_hist[1], y_hist[2]);
    } else {
        hist_init = false;  // 離したら履歴リセット
    }
}

/**
 * @brief タッチコントローラを初期化する
 *
 * @param i2c PL側I2Cコア
 * @return 初期化成功時 true
 */
bool TouchCtrl::Init(I2cPlCore &i2c)
{
    LOG_SCOPE();

    // 0サイズだと座標変換で破綻するので弾く
    if ((cfg_.width == 0u) || (cfg_.height == 0u)) {
        LOGE("Invalid touch cfg size: width=%u height=%u", (unsigned)cfg_.width, (unsigned)cfg_.height);
        return false;
    }

    i2c_ = &i2c;

    // タッチICをリセットして起動待ち
    i2c_->GpioOutput(0);  // RST=Low
    usleep(10'000);       // 10ms
    i2c_->GpioOutput(1);  // RST=High
    usleep(200'000);      // 起動安定待ち 200ms

    if (!i2c_->SetSlaveAddr(cfg_.addr)) {
        LOGE("Unmatched slave addr");
        return false;
    }

    // 最初はpendingをfalseに
    pending_.store(false, std::memory_order_release);
    last_point_ = {};

    return true;
}

/**
 * @brief タッチ更新処理
 *
 * @return 実行結果（IRQ再有効化要否を含む）
 */
TouchCtrl::RunResult TouchCtrl::Run()
{
    // IRQ起点で呼ばれたかを保持（再有効化判定に使う）
    const bool irq_pending = pending_.exchange(false, std::memory_order_acq_rel);

    // IRQが来ていたらここで受け取る
    bool need = irq_pending;

    // 押してる最中はIRQなしでも追従のため読む
    if (!need && last_point_.pressed) {
        need = true;
    }
    if (!need) {
        return RunResult::NoWork;
    }

    TouchPoint new_point{};
    if (!ReadTouchState(new_point)) {
        // 失敗時に再試行できるよう、次回で再度読み取り
        if (irq_pending) {
            pending_.store(true, std::memory_order_release);
        }
        return RunResult::NoWork;
    }

    MedianFilter3(&new_point);
    last_point_ = new_point;

    if (irq_pending) {
        return RunResult::NeedRearmIrq;
    }

    return RunResult::Updated;
}

/**
 * @brief タッチコントローラから1回分の状態を読み取る
 *
 * @param[out] out_point 読み取った座標・押下状態
 * @return 成功時 true
 */
bool TouchCtrl::ReadTouchState(TouchPoint &out_point)
{
    if (!i2c_) {
        return false;
    }

    // まずタッチ状態レジスタを読む
    uint8_t n_touch = 0;
    bool    ok      = false;
    // 一瞬のI2C取りこぼしはリトライで吸収
    for (uint8_t i = 0; i < kTouchReadRetryCount; ++i) {
        if (i2c_->ReadReg(kRegTouchStatus, &n_touch, 1)) {
            ok = true;
            break;
        }
        usleep(kTouchReadRetryUs);
    }

    if (!ok) {
        static uint32_t fail_count = 0;
        ++fail_count;
        if ((fail_count % kTouchErrLogInterval) == 1u) {
            LOGW("I2C read touch status failed (count=%lu)", (unsigned long)fail_count);
        }
        return false;
    }

    const bool      pressed        = (n_touch & 0x0F) > 0;
    static uint32_t ack_fail_count = 0;

    // 離し側は少しだけ粘ってチャタを吸収
    static uint8_t rel_cnt = 0;
    if (!pressed) {
        // すぐ離した判定にせず、少しだけ押下継続扱いにする
        if (rel_cnt < kReleaseDebounceCount) {
            rel_cnt++;
        }
        bool hold = (rel_cnt < kReleaseDebounceCount);

        uint8_t zero = 0;  // ステータスACKクリア
        if (!i2c_->WriteReg(kRegTouchStatus, &zero, 1)) {
            ++ack_fail_count;
            if ((ack_fail_count % kTouchErrLogInterval) == 1u) {
                LOGW("I2C clear touch status failed (count=%lu)", (unsigned long)ack_fail_count);
            }
            return false;
        }

        // 離し中は最後の座標をそのまま返す
        out_point = TouchPoint{last_point_.x, last_point_.y, hold};
        return true;
    }

    // 押しているので離しカウントはリセット
    rel_cnt = 0;

    // 1点分の生データをまとめて読む
    uint8_t buf[kTouchDataBlockSize] = {};
    if (!i2c_->ReadReg(kRegTouchData, buf, sizeof(buf))) {
        LOGE("I2C read touch data failed");
        return false;
    }

    // 読み終わったのでステータスACKクリア
    uint8_t zero = 0;
    if (!i2c_->WriteReg(kRegTouchStatus, &zero, 1)) {
        ++ack_fail_count;
        if ((ack_fail_count % kTouchErrLogInterval) == 1u) {
            LOGW("I2C clear touch status failed (count=%lu)", (unsigned long)ack_fail_count);
        }
        return false;
    }

    // マーカ違いは警告だけ出して先に進む
    if (buf[kFrameMarkerIndex] != kFrameMarkerValue) {
        LOGW("Frame marker missing: %02X", buf[kFrameMarkerIndex]);
    }

    // 生データを12bit座標へ復元
    uint16_t x = (static_cast<uint16_t>(buf[1]) << 4) | (buf[3] & 0x0F);
    uint16_t y = (static_cast<uint16_t>(buf[2]) << 4) | (buf[3] >> 4);

    TouchPoint point{x, y, true};  // ここではまだ生座標

    // 座標変換
    MapRawToScreen(point);

    out_point   = point;
    last_point_ = point;

    //LOGD("n=%u raw=(%u,%u) -> (%u,%u)", (unsigned)(n_touch & 0x0F), x, y, out_point.x, out_point.y);

    return true;
}

}  // namespace platform
}  // namespace core1
