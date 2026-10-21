#pragma once
#include "BitmapText.h"
#include "GameSession.h"
#include "KamataEngine.h"

// ステージ開始前に「STAGE 1」「LIFE *3」を表示する画面
class StageIntroScene {
public:
	~StageIntroScene();

	void Initialize(const GameSession* session);
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	// 表示する時間（秒）
	static inline const float kDuration = 2.0f;

	const GameSession* session_ = nullptr;

	KamataEngine::Sprite* background_ = nullptr;
	BitmapText text_;

	float timer_ = 0.0f;
	bool finished_ = false;
};
