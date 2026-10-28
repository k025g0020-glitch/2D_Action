#define NOMINMAX
#include "ResultScene.h"
#include "BgmPlayer.h"
#include "SoundEffects.h"
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

	// クリア（最後のステージなら全クリア）とゲームオーバーで曲を変える
	if (type_ == Type::kGameOver) {
		BgmPlayer::Play(BgmPlayer::Id::kGameOver);
	} else if (session_->IsLastStage()) {
		BgmPlayer::Play(BgmPlayer::Id::kAllClear);
	} else {
		BgmPlayer::Play(BgmPlayer::Id::kResult);
	}

	// 画面全体を黒で塗りつぶす
	background_ = Sprite::Create(TextureManager::Load("white1x1.png"), {0.0f, 0.0f});
	background_->SetSize({1280.0f, 720.0f});
	background_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});

	text_.Initialize(256);

	fade_ = new Fade();
	fade_->Initialize();

	// 選べること：クリアしたら「次のステージ（最後のステージでなければ）・再挑戦・終了」、ゲームオーバーなら「再挑戦・終了」
	choices_.clear();
	std::vector<VolumeMenu::Row> rows;
	if (type_ == Type::kStageClear && !session_->IsLastStage()) {
		choices_.push_back(Choice::kNextStage);
		rows.push_back({VolumeMenu::RowType::kAction, "次のステージへ"});
	}
	choices_.push_back(Choice::kRetry);
	rows.push_back({VolumeMenu::RowType::kAction, "再挑戦"});
	choices_.push_back(Choice::kQuit);
	rows.push_back({VolumeMenu::RowType::kAction, "タイトルへ戻る"});
	menu_.Initialize(rows);
	menu_.Open(0);
	choice_ = Choice::kQuit;

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

	case Phase::kWait: {
		const int selected = menu_.Update();
		if (selected >= 0) {
			choice_ = choices_[selected];
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
		}
		break;
	}

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

	// 入力待ちになったら、次にどうするかのメニューを出す
	if (phase_ != Phase::kTally) {
		const float top = (type_ == Type::kStageClear) ? 445.0f : 400.0f;
		menu_.Draw(text_, "", top, blinkTimer_, false);
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
		text_.Print("全ステージ クリア！", 640.0f, 110.0f, 56.0f, kYellow, BitmapText::Align::kCenter);
	} else {
		std::snprintf(buffer, sizeof(buffer), "ステージ%u クリア！", session_->stageIndex + 1);
		text_.Print(buffer, 640.0f, 110.0f, 56.0f, kYellow, BitmapText::Align::kCenter);
	}

	const StageResult& result = session_->lastStageResult;

	text_.Print("コイン", kLabelX, 240.0f, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%u", result.coins);
	text_.Print(buffer, kValueX, 240.0f, kCharSize, kWhite, BitmapText::Align::kRight);

	text_.Print("倒した敵", kLabelX, 300.0f, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%u", result.defeatedEnemies);
	text_.Print(buffer, kValueX, 300.0f, kCharSize, kWhite, BitmapText::Align::kRight);

	text_.Print("タイムボーナス", kLabelX, 360.0f, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%u×%u", remainingTime_, kTimeBonusPerSecond);
	text_.Print(buffer, kValueX, 360.0f, kCharSize, kWhite, BitmapText::Align::kRight);

	text_.Print("スコア", kLabelX, 460.0f, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%06u", session_->score);
	text_.Print(buffer, kValueX, 460.0f, kCharSize, kYellow, BitmapText::Align::kRight);
}

void ResultScene::DrawGameOver() {
	char buffer[32];

	text_.Print("ゲームオーバー", 640.0f, 240.0f, 72.0f, {1.0f, 0.3f, 0.25f, 1.0f}, BitmapText::Align::kCenter);

	std::snprintf(buffer, sizeof(buffer), "スコア %06u", session_->score);
	text_.Print(buffer, 640.0f, 400.0f, 36.0f, {1.0f, 1.0f, 1.0f, 1.0f}, BitmapText::Align::kCenter);
}
