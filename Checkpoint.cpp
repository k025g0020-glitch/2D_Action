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
}

void Checkpoint::Draw() { model_->Draw(worldTransform_, *camera_, &objectColor_); }

bool Checkpoint::IsPassedBy(const Vector3& playerPosition) const { return playerPosition.x >= worldTransform_.translation_.x; }

void Checkpoint::Activate() {
	if (isActivated_) {
		return;
	}
	isActivated_ = true;
	popTimer_ = kPopDuration;
	objectColor_.SetColor(kActiveColor);
}
