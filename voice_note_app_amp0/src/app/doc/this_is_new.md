了解です 👍
**「今の方針設計」**をそのまま README / 設計書にコピペできる Markdown と、
Excel にそのまま貼れる状態遷移表（TSV風） の両方を出します。

以下は 実装前の最終設計合意版として使える内容です。

⸻

AudioSystem 設計方針（簡易版・最終）

1. 設計ゴール
	•	豪華な仕様にしない
	•	再生中は再生のみ、録音中は録音のみ
	•	再生 ↔ 録音の即時切り替えはしない
	•	Stop / Error を「状態」として持たない
	•	状態数・分岐数を最小化する

⸻

2. レイヤ責務の整理

AudioSystem
	•	システム全体の 司令塔
	•	FSM にイベントを渡し、返ってきた Action を実行するだけ
	•	再生・録音の詳細ロジックは 一切持たない

AudioFsm
	•	状態遷移のみを担当
	•	副作用（Start/Stop/Pauseなど）は実行しない
	•	「次状態」と「やるべき Action」を返すだけ

PlaybackController
	•	再生のやり方担当
	•	DMA / AF / feeder / drain / pause / resume
	•	自然終了を内部イベントとして通知

RecordController
	•	録音のやり方担当
	•	DMA / harvest / recorder / target_bytes
	•	自然終了を内部イベントとして通知

⸻

3. 状態定義（最小）

enum class AudioState : uint8_t
{
    Idle = 0,     // 再生も録音もしていない
    Playing,     // 再生中
    Paused,      // 再生一時停止中
    Recording,   // 録音中
};

状態の考え方
	•	Idle は「再生も録音もしていない」唯一の共通状態
	•	Prepared / Draining / Stopping などの中間状態は Controller 内部に閉じる
	•	UI が知りたい状態だけを持つ

⸻

4. 入力イベント（FSMに渡すもの）

enum class AudioFsmEvent : uint8_t
{
    UiPlayPressed = 0,  // 再生トグル
    UiRecPressed,       // 録音トグル
    PbEnded,            // 再生が自然終了
    RecEnded,           // 録音が自然終了
};

ポイント
	•	Stop ボタンが無いので UiStopPressed は不要
	•	Error は FSM に入れない（System が直接処理）

⸻

5. アクション（FSMが指示する「やること」）

enum class AudioAction : uint16_t
{
    None = 0,

    StartPlayback,
    PausePlayback,
    ResumePlayback,

    StartRecord,
    StopRecord,

    EmitPlaybackStarted,
    EmitPlaybackPaused,
    EmitPlaybackResumed,
    EmitPlaybackStopped,
    EmitRecordStarted,
    EmitRecordStopped,
};

ポイント
	•	Action は 副作用
	•	実行するのは AudioSystem
	•	FSM は「指示」しかしない

⸻

6. 状態遷移表（Markdown）

Idle

Event	Next State	Action
UiPlayPressed	Playing	StartPlayback, EmitPlaybackStarted
UiRecPressed	Recording	StartRecord, EmitRecordStarted
PbEnded	Idle	None
RecEnded	Idle	None


⸻

Playing

Event	Next State	Action
UiPlayPressed	Paused	PausePlayback, EmitPlaybackPaused
UiRecPressed	Playing	None（無効）
PbEnded	Idle	EmitPlaybackStopped
RecEnded	Playing	None


⸻

Paused

Event	Next State	Action
UiPlayPressed	Playing	ResumePlayback, EmitPlaybackResumed
UiRecPressed	Paused	None（無効）
PbEnded	Idle	EmitPlaybackStopped
RecEnded	Paused	None


⸻

Recording

Event	Next State	Action
UiPlayPressed	Recording	None（無効）
UiRecPressed	Idle	StopRecord, EmitRecordStopped
PbEnded	Recording	None
RecEnded	Idle	EmitRecordStopped


⸻

7. 状態遷移表（Excel貼り付け用）

以下は そのままコピー → Excelに貼り付け可（TSV形式）

CurrentState	Event	NextState	Action1	Action2
Idle	UiPlayPressed	Playing	StartPlayback	EmitPlaybackStarted
Idle	UiRecPressed	Recording	StartRecord	EmitRecordStarted
Idle	PbEnded	Idle	None	None
Idle	RecEnded	Idle	None	None

Playing	UiPlayPressed	Paused	PausePlayback	EmitPlaybackPaused
Playing	UiRecPressed	Playing	None	None
Playing	PbEnded	Idle	None	EmitPlaybackStopped
Playing	RecEnded	Playing	None	None

Paused	UiPlayPressed	Playing	ResumePlayback	EmitPlaybackResumed
Paused	UiRecPressed	Paused	None	None
Paused	PbEnded	Idle	None	EmitPlaybackStopped
Paused	RecEnded	Paused	None	None

Recording	UiPlayPressed	Recording	None	None
Recording	UiRecPressed	Idle	StopRecord	EmitRecordStopped
Recording	PbEnded	Recording	None	None
Recording	RecEnded	Idle	None	EmitRecordStopped


⸻

8. この設計のメリット
	•	状態が 4つだけで理解しやすい
	•	AudioSystem の switch / if が激減する
	•	Controller が完全に「実装詳細」になる
	•	UI と FSM が 1:1 で対応する
	•	将来「Stopボタン追加」「エラー画面追加」も FSM拡張だけで可能

⸻

次のステップは
👉 この設計に合わせて fsm.h を整理した完成版
👉 AudioSystem を FSMドリブンに書き直した最小実装

どちらから行きますか？
