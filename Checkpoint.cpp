#include "Checkpoint.h"
#include "calc.h"
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void Checkpoint::Initialize(Model* model, Camera* camera, const Vector3& position, bool activated) {
	assert(model);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	objectColor_.Initialize();

	isActivated_ = activated;
	popTimer_ = 0.0f;
	objectColor_.SetColor(isActivated_ ? kActiveColor : kInactiveColor);
}

void Checkpoint::SetFlagModel(Model* flagModel) {
	assert(flagModel);
	flagModel_ = flagModel;
	flagTransform_.Initialize();
}

void Checkpoint::Update() {
	// 通過直後は旗がはずむように伸び縮みする
	if (popTimer_ > 0.0f) {
		popTimer_ -= 1.0f / 60.0f;
		if (popTimer_ < 0.0f) {
			popTimer_ = 0.0f;
		}
		const float t = 1.0f - popTimer_ / kPopDuration;
		const float scale = 1.0f + std::sin(t * std::numbers::pi_v<float>) * kPopScale;
		worldTransform_.scale_ = {scale, scale, scale};
	}

	worldTransform_.matWorld_ = calc::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	if (flagModel_) {
		// 旗はポールの付け根を軸にして、風でゆっくり揺れる
		waveTimer_ += 1.0f / 60.0f;
		const float wave = std::sin(waveTimer_ * kFlagWaveSpeed) * kFlagWaveAngle;
		flagTransform_.matWorld_ = calc::Multiply(calc::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, wave, 0.0f}, kFlagPivot), worldTransform_.matWorld_);
		flagTransform_.TransferMatrix();
	}
}

void Checkpoint::Draw() {
	if (!flagModel_) {
		model_->Draw(worldTransform_, *camera_, &objectColor_);
		return;
	}
	// ポールはいつも同じ明るさ。旗だけ通過前は暗く、通過後は明るくする
	model_->Draw(worldTransform_, *camera_);
	flagModel_->Draw(flagTransform_, *camera_, &objectColor_);
}

bool Checkpoint::IsPassedBy(const Vector3& playerPosition) const { return playerPosition.x >= worldTransform_.translation_.x; }

void Checkpoint::Activate() {
	if (isActivated_) {
		return;
	}
	isActivated_ = true;
	popTimer_ = kPopDuration;
	objectColor_.SetColor(kActiveColor);
}
