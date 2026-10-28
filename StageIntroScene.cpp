#include "StageIntroScene.h"
#include "BgmPlayer.h"
#include "GameScene.h"
#include <cassert>
#include <cstdio>

using namespace KamataEngine;

StageIntroScene::~StageIntroScene() { delete background_; }

void StageIntroScene::Initialize(const GameSession* session) {
	assert(session);
	session_ = session;

	// ステージ名を見せる間は曲を止めて、ゲームが始まったらステージの曲を流す
	BgmPlayer::Stop();

	// 画面全体を黒で塗りつぶす
	background_ = Sprite::Create(TextureManager::Load("white1x1.png"), {0.0f, 0.0f});
	background_->SetSize({1280.0f, 720.0f});
	background_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});

	text_.Initialize(256);

	timer_ = 0.0f;
	finished_ = false;
}

void StageIntroScene::Update() {
	timer_ += 1.0f / 60.0f;
	if (timer_ >= kDuration) {
		finished_ = true;
	}
}

void StageIntroScene::Draw() {
	Sprite::PreDraw();
	background_->Draw();
	Sprite::PostDraw();

	char buffer[128];

	text_.Clear();

	std::snprintf(buffer, sizeof(buffer), "ステージ %u", session_->stageIndex + 1);
	text_.Print(buffer, 640.0f, 220.0f, 64.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);
	std::snprintf(buffer, sizeof(buffer), "残機 × %d", session_->lives);
	text_.Print(buffer, 640.0f, 320.0f, 40.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);

	// 残機が増える条件
	const Vector4 kGray = {0.75f, 0.78f, 0.85f, 1.0f};
	std::snprintf(buffer, sizeof(buffer), "コインを%u枚集めると 残機+1", GameSession::kCoinsForExtraLife);
	text_.Print(buffer, 640.0f, 380.0f, 26.0f, kGray, BitmapText::Align::kCenter);
	std::snprintf(buffer, sizeof(buffer), "着地せずに敵を%u体続けて踏むと 残機+1", GameScene::StompsForExtraLife());
	text_.Print(buffer, 640.0f, 416.0f, 26.0f, kGray, BitmapText::Align::kCenter);

	if (session_->HasCheckpoint()) {
		text_.Print("中間ポイントから再開", 640.0f, 470.0f, 28.0f, {1.0f, 0.5f, 0.4f, 1.0f}, BitmapText::Align::kCenter);
	}

	// ステージごとのヒント
	if (const char* hint = session_->GetStage().hint) {
		text_.Print(hint, 640.0f, 540.0f, 28.0f, {1.0f, 0.9f, 0.3f, 1.0f}, BitmapText::Align::kCenter);
	}

	text_.Draw();
}
