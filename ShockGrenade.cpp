#include "ShockGrenade.h"
#include "MapChipField.h"
#include "calc.h"
#include <algorithm>
#include <cassert>
#include <cmath>

using namespace KamataEngine;

void ShockGrenade::Initialize(Model* model, Camera* camera, MapChipField* mapChipField, const Vector3& origin, const Vector3& target) {
	assert(model);
	assert(mapChipField);

	model_ = model;
	camera_ = camera;
	mapChipField_ = mapChipField;

	worldTransform_.Initialize();
	worldTransform_.translation_ = origin;

	// 飛んでいる時間を決めて、ちょうど target を通るように初速を逆算する（放物線）
	//   x : origin.x + vx * t
	//   y : origin.y + vy * t - g * t^2 / 2
	const float dx = target.x - origin.x;
	const float dy = target.y - origin.y;
	const float frames = std::clamp(std::abs(dx) / kHorizontalSpeed, kMinFlightFrames, kMaxFlightFrames);
	velocity_.x = dx / frames;
	velocity_.y = (dy + 0.5f * kGravity * frames * frames) / frames;

	lifeTimer_ = 0.0f;
	isExploded_ = false;
	isGone_ = false;
}

void ShockGrenade::Update() {
	if (isExploded_ || isGone_) {
		return;
	}

	velocity_.y -= kGravity;
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	// 進む向きに回転させる
	worldTransform_.rotation_.z -= (velocity_.x >= 0.0f ? 1.0f : -1.0f) * kSpinSpeed;

	// ブロックに当たったら爆発
	const MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y) == MapChipType::kBlock) {
		isExploded_ = true;
	}

	lifeTimer_ += 1.0f / 60.0f;
	if (worldTransform_.translation_.y < kFallOutY || lifeTimer_ > kLifeTime) {
		isGone_ = true;
	}

	worldTransform_.matWorld_ = calc::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void ShockGrenade::Draw() {
	if (isExploded_ || isGone_) {
		return;
	}
	model_->Draw(worldTransform_, *camera_);
}

AABB ShockGrenade::GetAABB() const {
	const Vector3& position = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {position.x - kSize / 2.0f, position.y - kSize / 2.0f, position.z - kSize / 2.0f};
	aabb.max = {position.x + kSize / 2.0f, position.y + kSize / 2.0f, position.z + kSize / 2.0f};
	return aabb;
}
