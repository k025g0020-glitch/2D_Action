#include "Coin.h"
#include "calc.h"
#include <cassert>

using namespace KamataEngine;

void Coin::Initialize(Model* model, Camera* camera, const Vector3& position) {
	assert(model);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	state_ = State::kIdle;
}

void Coin::Update() {
	switch (state_) {
	case State::kIdle:
		worldTransform_.rotation_.y += kRotateSpeed;
		break;

	case State::kCollected:
		// 高速回転しながらポンと跳ねて消える
		worldTransform_.rotation_.y += kCollectedRotateSpeed;
		worldTransform_.translation_.y += velocityY_;
		velocityY_ -= kCollectedGravity;

		collectedTimer_ += 1.0f / 60.0f;
		if (collectedTimer_ >= kCollectedDuration) {
			state_ = State::kGone;
		}
		break;

	case State::kGone:
		return;
	}

	worldTransform_.matWorld_ = calc::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Coin::Draw() {
	if (state_ == State::kGone) {
		return;
	}
	model_->Draw(worldTransform_, *camera_);
}

AABB Coin::GetAABB() const {
	const Vector3& pos = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {pos.x - kSize / 2.0f, pos.y - kSize / 2.0f, pos.z - kSize / 2.0f};
	aabb.max = {pos.x + kSize / 2.0f, pos.y + kSize / 2.0f, pos.z + kSize / 2.0f};
	return aabb;
}

void Coin::Collect() {
	if (state_ != State::kIdle) {
		return;
	}
	state_ = State::kCollected;
	velocityY_ = kCollectedJumpSpeed;
	collectedTimer_ = 0.0f;
}
