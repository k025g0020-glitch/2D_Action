#include "StageIntroScene.h"
#include <cassert>
#include <cstdio>

using namespace KamataEngine;

StageIntroScene::~StageIntroScene() { delete background_; }

void StageIntroScene::Initialize(const GameSession* session) {
	assert(session);
	session_ = session;

	// 画面全体を黒で塗りつぶす
	background_ = Sprite::Create(TextureManager::Load("white1x1.png"), {0.0f, 0.0f});
	background_->SetSize({1280.0f, 720.0f});
	background_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});

	text_.Initialize(64);

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

	char buffer[32];

	text_.Clear();

	std::snprintf(buffer, sizeof(buffer), "STAGE %u", session_->stageIndex + 1);
	text_.Print(buffer, 640.0f, 220.0f, 64.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);
	// 街の名前
	text_.Print(session_->GetStage().name, 640.0f, 300.0f, 32.0f, {1.0f, 0.75f, 0.45f, 1.0f}, BitmapText::Align::kCenter);

	std::snprintf(buffer, sizeof(buffer), "LIFE * %d", session_->lives);
	text_.Print(buffer, 640.0f, 380.0f, 40.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);

	if (session_->checkpointReached) {
		text_.Print("FROM CHECKPOINT", 640.0f, 450.0f, 28.0f, {1.0f, 0.5f, 0.4f, 1.0f}, BitmapText::Align::kCenter);
	}

	// ステージごとのヒント
	if (const char* hint = session_->GetStage().hint) {
		text_.Print(hint, 640.0f, 540.0f, 28.0f, {1.0f, 0.9f, 0.3f, 1.0f}, BitmapText::Align::kCenter);
	}

	text_.Draw();
}
