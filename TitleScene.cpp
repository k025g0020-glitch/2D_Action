#include "TitleScene.h"
#include <cmath>

using namespace KamataEngine;

TitleScene::~TitleScene() {
	// フェードの解放
	delete fade_;
}

void TitleScene::Initialize() {
	finished_ = false;

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	text_.Initialize(128);
	blinkTimer_ = 0.0f;

	// 初期フェーズ設定とフェードイン開始
	phase_ = Phase::kFadeIn;
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);
}

void TitleScene::Update() {
	// フェードの更新
	if (fade_) {
		fade_->Update();
	}

	blinkTimer_ += 1.0f / 60.0f;

	switch (phase_) {
	case Phase::kFadeIn:
		// フェードインが終了したらメインフェーズへ移行
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// スペースキーでフェードアウトフェーズへ移行
		if (Input::GetInstance()->PushKey(DIK_SPACE)) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
		}
		break;

	case Phase::kFadeOut:
		// フェードアウトが終了したらシーン終了フラグを立てる
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

void TitleScene::Draw() {
	text_.Clear();

	// 「PRESS SPACE」を点滅させる
	if (std::fmod(blinkTimer_, 1.0f) < 0.6f) {
		text_.Print("PRESS SPACE TO START", 640.0f, 400.0f, 40.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);
	}
	text_.Print("A / D : MOVE", 640.0f, 520.0f, 28.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);
	text_.Print("SPACE / W : JUMP", 640.0f, 560.0f, 28.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);
	text_.Print("SHIFT : DASH", 640.0f, 600.0f, 28.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);

	text_.Draw();

	// フェードの描画
	if (fade_) {
		fade_->Draw();
	}
}