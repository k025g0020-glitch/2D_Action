#include "TitleScene.h"
#include "BgmPlayer.h"
#include "SoundEffects.h"
#include "CameraController.h"
#include <cmath>

using namespace KamataEngine;

TitleScene::~TitleScene() {
	// フェードの解放
	delete fade_;
}

void TitleScene::Initialize() {
	finished_ = false;

	BgmPlayer::Play(BgmPlayer::Id::kTitle);

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	text_.Initialize(128);
	blinkTimer_ = 0.0f;

	// ゲーム画面と同じ高さ・距離から街並みを映す
	camera_.Initialize();
	camera_.translation_ = {kCameraStartX, 6.0f, -15.0f};
	cityBackground_.Initialize(&camera_, kCityWidth, 2024);

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

	// 背景をゆっくり横に流す（端まで来たら最初に戻す）
	camera_.translation_.x += kScrollSpeed;
	if (camera_.translation_.x > kCityWidth - kCameraStartX) {
		camera_.translation_.x = kCameraStartX;
	}
	camera_.UpdateMatrix();
	CameraController::ApplyOrthographic(&camera_);
	cityBackground_.Update();

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
			SoundEffects::Play(SoundEffects::Id::kDecide);
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
	Model::PreDraw();
	cityBackground_.Draw();
	Model::PostDraw();

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