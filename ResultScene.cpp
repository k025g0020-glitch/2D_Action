#define NOMINMAX
#include "ResultScene.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace KamataEngine;

ResultScene::~ResultScene() {
	delete background_;
	delete fade_;
}

void ResultScene::Initialize(GameSession* session, Type type) {
	assert(session);
	session_ = session;
	type_ = type;

	// 画面全体を黒で塗りつぶす
	background_ = Sprite::Create(TextureManager::Load("white1x1.png"), {0.0f, 0.0f});
	background_->SetSize({1280.0f, 720.0f});
	background_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});

	text_.Initialize(256);

	fade_ = new Fade();
	fade_->Initialize();

	remainingTime_ = session_->lastStageResult.remainingTime;
	phase_ = (type_ == Type::kStageClear) ? Phase::kTally : Phase::kWait;
	blinkTimer_ = 0.0f;
	finished_ = false;
}

void ResultScene::Update() {
	Input* input = Input::GetInstance();

	fade_->Update();
	blinkTimer_ += 1.0f / 60.0f;

	switch (phase_) {
	case Phase::kTally: {
		// SPACEで残りを一気に加算する
		const uint32_t seconds = input->TriggerKey(DIK_SPACE) ? remainingTime_ : std::min(remainingTime_, kTallySecondsPerFrame);
		remainingTime_ -= seconds;
		session_->AddScore(seconds * kTimeBonusPerSecond);

		if (remainingTime_ == 0) {
			phase_ = Phase::kWait;
			blinkTimer_ = 0.0f;
		}
		break;
	}

	case Phase::kWait:
		if (input->TriggerKey(DIK_SPACE)) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
		}
		break;

	case Phase::kFadeOut:
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

void ResultScene::Draw() {
	Sprite::PreDraw();
	background_->Draw();
	Sprite::PostDraw();

	text_.Clear();

	if (type_ == Type::kStageClear) {
		DrawStageClear();
	} else {
		DrawGameOver();
	}

	// 入力待ちの間は「PRESS SPACE」を点滅させる
	if (phase_ == Phase::kWait && std::fmod(blinkTimer_, 1.0f) < 0.6f) {
		text_.Print("PRESS SPACE", 640.0f, 610.0f, 32.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);
	}

	text_.Draw();

	fade_->Draw();
}

void ResultScene::DrawStageClear() {
	const Vector4 kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
	const Vector4 kYellow = {1.0f, 0.9f, 0.3f, 1.0f};
	constexpr float kCharSize = 36.0f;
	constexpr float kLabelX = 340.0f;
	constexpr float kValueX = 940.0f;

	char buffer[48];

	if (session_->IsLastStage()) {
		text_.Print("ALL STAGES CLEAR!", 640.0f, 110.0f, 56.0f, kYellow, BitmapText::Align::kCenter);
	} else {
		std::snprintf(buffer, sizeof(buffer), "STAGE %u CLEAR!", session_->stageIndex + 1);
		text_.Print(buffer, 640.0f, 110.0f, 56.0f, kYellow, BitmapText::Align::kCenter);
	}

	const StageResult& result = session_->lastStageResult;

	text_.Print("COIN", kLabelX, 240.0f, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%u", result.coins);
	text_.Print(buffer, kValueX, 240.0f, kCharSize, kWhite, BitmapText::Align::kRight);

	text_.Print("ENEMY", kLabelX, 300.0f, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%u", result.defeatedEnemies);
	text_.Print(buffer, kValueX, 300.0f, kCharSize, kWhite, BitmapText::Align::kRight);

	text_.Print("TIME BONUS", kLabelX, 360.0f, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%u*%u", remainingTime_, kTimeBonusPerSecond);
	text_.Print(buffer, kValueX, 360.0f, kCharSize, kWhite, BitmapText::Align::kRight);

	text_.Print("SCORE", kLabelX, 460.0f, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%06u", session_->score);
	text_.Print(buffer, kValueX, 460.0f, kCharSize, kYellow, BitmapText::Align::kRight);
}

void ResultScene::DrawGameOver() {
	char buffer[32];

	text_.Print("GAME OVER", 640.0f, 240.0f, 72.0f, {1.0f, 0.3f, 0.25f, 1.0f}, BitmapText::Align::kCenter);

	std::snprintf(buffer, sizeof(buffer), "SCORE %06u", session_->score);
	text_.Print(buffer, 640.0f, 400.0f, 36.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);
}
