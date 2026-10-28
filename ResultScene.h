#pragma once
#include "BitmapText.h"
#include "Fade.h"
#include "GameSession.h"
#include "KamataEngine.h"
#include "VolumeMenu.h"
#include <vector>

// ステージクリア時の結果画面・ゲームオーバー画面
class ResultScene {
public:
	enum class Type {
		kStageClear, // ステージクリア（残り時間をボーナスとして加算する）
		kGameOver,   // ゲームオーバー
	};

	// 結果画面で選んだこと
	enum class Choice {
		kNextStage, // 次のステージへ
		kRetry,     // 同じステージにもう一度挑戦する
		kQuit,      // タイトルに戻る
	};

	~ResultScene();

	void Initialize(GameSession* session, Type type);
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }
	Choice GetChoice() const { return choice_; }

private:
	enum class Phase {
		kTally,   // 残り時間をスコアに加算している途中
		kWait,    // 入力待ち
		kFadeOut, // フェードアウト
	};

	void DrawStageClear();
	void DrawGameOver();

	// 残り1秒あたりのボーナス点
	static inline const uint32_t kTimeBonusPerSecond = 50;
	// 1フレームに加算する秒数
	static inline const uint32_t kTallySecondsPerFrame = 1;
	static inline const float kFadeDuration = 0.5f;

	GameSession* session_ = nullptr;
	Type type_ = Type::kStageClear;
	Phase phase_ = Phase::kTally;

	// まだスコアに加算していない残り時間
	uint32_t remainingTime_ = 0;

	KamataEngine::Sprite* background_ = nullptr;
	Fade* fade_ = nullptr;
	BitmapText text_;

	float blinkTimer_ = 0.0f;
	bool finished_ = false;

	// 入力待ちの間に出すメニューと、各行で選べること
	VolumeMenu menu_;
	std::vector<Choice> choices_;
	Choice choice_ = Choice::kQuit;
};
