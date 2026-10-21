#include "GoalStar.h"
#include "calc.h"
#include <cassert>
#include <cmath>

using namespace KamataEngine;

void GoalStar::Initialize(Model* model, Camera* camera, const Vector3& position) {
	assert(model);

	model_ = model;
	camera_ = camera;
	basePosition_ = position;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	floatTimer_ = 0.0f;
	isCollected_ = false;
}

void GoalStar::Update() {
	if (!isCollected_) {
		// くるくる回りながら上下にふわふわ浮く
		worldTransform_.rotation_.y += kRotateSpeed;
		floatTimer_ += kFloatSpeed;
		worldTransform_.translation_.y = basePosition_.y + std::sin(floatTimer_) * kFloatAmplitude;
	} else {
		// 獲得演出：高速回転しながら上昇して大きくなる
		worldTransform_.rotation_.y += kCollectedRotateSpeed;
		worldTransform_.translation_.y += kCollectedRiseSpeed;
		if (worldTransform_.scale_.x < kCollectedMaxScale) {
			worldTransform_.scale_.x += kCollectedScaleSpeed;
			worldTransform_.scale_.y += kCollectedScaleSpeed;
			worldTransform_.scale_.z += kCollectedScaleSpeed;
		}
	}

	worldTransform_.matWorld_ = calc::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void GoalStar::Draw() { model_->Draw(worldTransform_, *camera_); }

AABB GoalStar::GetAABB() const {
	const Vector3& pos = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {pos.x - kWidth / 2.0f, pos.y - kHeight / 2.0f, pos.z - kWidth / 2.0f};
	aabb.max = {pos.x + kWidth / 2.0f, pos.y + kHeight / 2.0f, pos.z + kWidth / 2.0f};
	return aabb;
}

void GoalStar::Collect() { isCollected_ = true; }
