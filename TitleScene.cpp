#include "TitleScene.h"
#include "BgmPlayer.h"
#include "SoundEffects.h"
#include "SoundSettings.h"
#include "CameraController.h"
#include <cmath>
#include <cstdio>
#include <string>

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

	text_.Initialize(320);
	blinkTimer_ = 0.0f;
	menuItem_ = MenuItem::kStart;
	isSoundMenuOpen_ = false;

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
		if (isSoundMenuOpen_) {
			UpdateSoundMenu();
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

	text_.Clear();
	if (isSoundMenuOpen_) {
		DrawSoundMenu();
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

// 項目を上下に選ぶ（W / S）。端まで行ったら反対側に回る
template <typename T> T SelectItem(T current) {
	Input* input = Input::GetInstance();
	const int count = static_cast<int>(T::kCount);
	int index = static_cast<int>(current);
	if (input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP)) {
		index = (index + count - 1) % count;
	}
	if (input->TriggerKey(DIK_S) || input->TriggerKey(DIK_DOWN)) {
		index = (index + 1) % count;
	}
	return static_cast<T>(index);
}

// 左右の入力（A / D）。-1 / 0 / +1
int HorizontalInput() {
	Input* input = Input::GetInstance();
	int direction = 0;
	if (input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT)) {
		--direction;
	}
	if (input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT)) {
		++direction;
	}
	return direction;
}

// 「< =======--- >  70」のような音量の表示（左右の矢印で A / D で変えられることを示す）
std::string VolumeBar(int level) {
	std::string bar = "< ";
	for (int i = 0; i < SoundSettings::kMaxLevel; ++i) {
		bar += (i < level) ? '=' : '-';
	}
	char percent[8];
	std::snprintf(percent, sizeof(percent), "%4d", level * 100 / SoundSettings::kMaxLevel);
	return bar + " >" + percent;
}
} // namespace

void TitleScene::UpdateMenu() {
	const MenuItem previous = menuItem_;
	menuItem_ = SelectItem(menuItem_);
	if (menuItem_ != previous) {
		SoundEffects::Play(SoundEffects::Id::kCoin);
	}

	if (!Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		return;
	}
	SoundEffects::Play(SoundEffects::Id::kDecide);
	if (menuItem_ == MenuItem::kStart) {
		// ゲームを始める
		phase_ = Phase::kFadeOut;
		fade_->Start(Fade::Status::FadeOut, kFadeDuration);
	} else {
		isSoundMenuOpen_ = true;
		soundItem_ = SoundItem::kBgm;
	}
}

void TitleScene::UpdateSoundMenu() {
	const SoundItem previous = soundItem_;
	soundItem_ = SelectItem(soundItem_);
	if (soundItem_ != previous) {
		SoundEffects::Play(SoundEffects::Id::kCoin);
	}

	// A / D で音量を1段ずつ変える
	const int direction = HorizontalInput();
	if (direction != 0) {
		if (soundItem_ == SoundItem::kBgm) {
			SoundSettings::SetBgmLevel(SoundSettings::GetBgmLevel() + direction);
			// 流れている曲の音量をすぐ変えて、大きさを確かめられるようにする
			BgmPlayer::RefreshVolume();
		} else if (soundItem_ == SoundItem::kSe) {
			SoundSettings::SetSeLevel(SoundSettings::GetSeLevel() + direction);
			// 効果音を鳴らして大きさを確かめられるようにする
			SoundEffects::Play(SoundEffects::Id::kCoin);
		}
	}

	// BACK で戻る。戻る時に音量を保存する
	if (soundItem_ == SoundItem::kBack && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SoundSettings::Save();
		SoundEffects::Play(SoundEffects::Id::kDecide);
		isSoundMenuOpen_ = false;
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

	text_.Print("A / D : MOVE", 640.0f, 560.0f, 28.0f, kWhite, BitmapText::Align::kCenter);
	text_.Print("SPACE / W : JUMP", 640.0f, 600.0f, 28.0f, kWhite, BitmapText::Align::kCenter);
	text_.Print("SHIFT : DASH", 640.0f, 640.0f, 28.0f, kWhite, BitmapText::Align::kCenter);
}

void TitleScene::DrawSoundMenu() {
	text_.Print("SOUND", 640.0f, 320.0f, 44.0f, kWhite, BitmapText::Align::kCenter);

	const struct {
		const char* label;
		int level;
	} kRows[] = {
	    {"BGM", SoundSettings::GetBgmLevel()},
	    {"SE ", SoundSettings::GetSeLevel()},
	};
	constexpr float kRowX = 280.0f;
	for (int i = 0; i < 2; ++i) {
		const bool selected = static_cast<int>(soundItem_) == i;
		const std::string row = std::string(selected ? "> " : "  ") + kRows[i].label + "  " + VolumeBar(kRows[i].level);
		text_.Print(row, kRowX, 400.0f + 56.0f * i, 36.0f, selected ? kYellow : kWhite);
	}

	const bool backSelected = soundItem_ == SoundItem::kBack;
	text_.Print(backSelected ? "> BACK <" : "BACK", 640.0f, 530.0f, 36.0f, backSelected ? kYellow : kWhite, BitmapText::Align::kCenter);

	text_.Print("W / S : SELECT   A / D : VOLUME   SPACE : BACK", 640.0f, 610.0f, 24.0f, kGray, BitmapText::Align::kCenter);
}
