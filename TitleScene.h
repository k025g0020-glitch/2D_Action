#pragma once
#include "BitmapText.h"
#include "CityBackground.h"
#include "Fade.h"
#include "KamataEngine.h"

class TitleScene {
public:

	enum class Phase {
		kFadeIn,  // フェードイン
		kMain,    // メイン部
		kFadeOut, // フェードアウト
	};

	~TitleScene(); // デストラクタを追加

	void Initialize();
	void Update();
	void Draw();

	// 終了フラグのgetter
	bool IsFinished() const { return finished_; }

private:
	// フェード時間（秒）
	static inline const float kFadeDuration = 1.0f;

	// 現在のフェーズ
	Phase phase_ = Phase::kFadeIn;

	// 終了フラグ
	bool finished_ = false;

	// フェードオブジェクト
	Fade* fade_ = nullptr;

	// 背景の街並み（ゆっくり横に流れる）
	KamataEngine::Camera camera_;
	CityBackground cityBackground_;
	// 背景を流す速さと、街並みの長さ
	static inline const float kScrollSpeed = 0.03f;
	static inline const float kCityWidth = 300.0f;
	static inline const float kCameraStartX = 13.0f;

	// 操作説明などの文字
	BitmapText text_;
	float blinkTimer_ = 0.0f;
};