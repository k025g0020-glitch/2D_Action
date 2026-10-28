#include "TitleScene.h"
#include "BgmPlayer.h"
#include "SoundEffects.h"
#include "MenuInput.h"
#include "CameraController.h"
#include <cmath>
#include <string>

using namespace KamataEngine;

TitleScene::~TitleScene() {
	// フェードの解放
	delete fade_;
	delete logo_;
	delete logoGlow_;
}

void TitleScene::Initialize() {
	finished_ = false;

	BgmPlayer::Play(BgmPlayer::Id::kTitle);

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	// タイトルロゴ
	logoGlow_ = Sprite::Create(TextureManager::Load("title/logo_glow.png"), kLogoPosition);
	logo_ = Sprite::Create(TextureManager::Load("title/logo.png"), kLogoPosition);

	text_.Initialize(320);
	blinkTimer_ = 0.0f;
	menuItem_ = MenuItem::kStart;
	isSoundMenuOpen_ = false;
	volumeMenu_.Initialize({
	    {VolumeMenu::RowType::kBgm, "BGM"},
	    {VolumeMenu::RowType::kSe, "SE"},
	    {VolumeMenu::RowType::kAction, "BACK"},
	});

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

	// ロゴをゆっくり上下に浮かせ、ネオンの光を明るくしたり暗くしたりする（ときどき一瞬ちらつく）
	const Vector2 logoPosition = {kLogoPosition.x, kLogoPosition.y + std::sin(blinkTimer_ * kLogoFloatSpeed) * kLogoFloatHeight};
	logo_->SetPosition(logoPosition);
	logoGlow_->SetPosition(logoPosition);
	float glow = 0.75f + 0.25f * std::sin(blinkTimer_ * 3.0f);
	if (std::fmod(blinkTimer_, 4.0f) > 3.85f) {
		glow *= 0.4f;
	}
	logoGlow_->SetColor({1.0f, 1.0f, 1.0f, glow});

	switch (phase_) {
	case Phase::kFadeIn:
		// フェードインが終了したらメインフェーズへ移行
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		if (isSoundMenuOpen_) {
			// BACK で音量の設定を閉じる
			if (volumeMenu_.Update() >= 0) {
				isSoundMenuOpen_ = false;
			}
		} else {
			UpdateMenu();
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

	Sprite::PreDraw();
	logoGlow_->Draw();
	logo_->Draw();
	Sprite::PostDraw();

	text_.Clear();
	if (isSoundMenuOpen_) {
		volumeMenu_.Draw(text_, "SOUND", 320.0f, blinkTimer_);
	} else {
		DrawMenu();
	}
	text_.Draw();

	// フェードの描画
	if (fade_) {
		fade_->Draw();
	}
}

namespace {
const Vector4 kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
const Vector4 kYellow = {1.0f, 0.9f, 0.3f, 1.0f};
const Vector4 kGray = {0.6f, 0.62f, 0.7f, 1.0f};
} // namespace

void TitleScene::UpdateMenu() {
	const MenuItem previous = menuItem_;
	menuItem_ = MenuInput::SelectItem(menuItem_);
	if (menuItem_ != previous) {
		SoundEffects::Play(SoundEffects::Id::kCoin);
	}

	if (!MenuInput::Decide()) {
		return;
	}
	SoundEffects::Play(SoundEffects::Id::kDecide);
	if (menuItem_ == MenuItem::kStart) {
		// ゲームを始める
		phase_ = Phase::kFadeOut;
		fade_->Start(Fade::Status::FadeOut, kFadeDuration);
	} else {
		isSoundMenuOpen_ = true;
		volumeMenu_.Open();
	}
}

void TitleScene::DrawMenu() {
	static const char* const kLabels[] = {"START", "SOUND"};
	for (int i = 0; i < static_cast<int>(MenuItem::kCount); ++i) {
		const bool selected = static_cast<int>(menuItem_) == i;
		// 選んでいる項目は黄色にして、矢印を点滅させる
		std::string label = kLabels[i];
		if (selected && std::fmod(blinkTimer_, 1.0f) < 0.6f) {
			label = "> " + label + " <";
		}
		text_.Print(label, 640.0f, 380.0f + 52.0f * i, 40.0f, selected ? kYellow : kWhite, BitmapText::Align::kCenter);
	}
	text_.Print("W / S : SELECT   SPACE : OK", 640.0f, 490.0f, 24.0f, kGray, BitmapText::Align::kCenter);

	text_.Print("A / D : MOVE", 640.0f, 550.0f, 28.0f, kWhite, BitmapText::Align::kCenter);
	text_.Print("SPACE / W : JUMP", 640.0f, 590.0f, 28.0f, kWhite, BitmapText::Align::kCenter);
	text_.Print("SHIFT : DASH", 640.0f, 630.0f, 28.0f, kWhite, BitmapText::Align::kCenter);
	text_.Print("ESC : PAUSE", 640.0f, 670.0f, 28.0f, kWhite, BitmapText::Align::kCenter);
}
