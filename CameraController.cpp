#include "CameraController.h"
#include <algorithm>
#include <cmath>

using namespace KamataEngine;


void CameraController::Initialize(KamataEngine::Camera* camera) {
	camera_ = camera;
}

void CameraController::Reset() {
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	lookAheadOffsetX_ = facingDirection_ * kLookAheadDistance;
	camera_->translation_.x = std::clamp(targetWorldTransform.translation_.x + lookAheadOffsetX_ + targetOffset_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(CalculateCameraY(targetWorldTransform.translation_.y) + targetOffset_.y, movableArea_.bottom, movableArea_.top);
	camera_->translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;
	camera_->UpdateMatrix();
	ApplyOrthographicProjection();
}

void CameraController::ApplyOrthographicProjection() {
	const float halfHeight = GetViewHalfHeight();
	const float halfWidth = GetViewHalfWidth();
	camera_->matProjection = calc::MakeOrthographicMatrix(-halfWidth, halfHeight, halfWidth, -halfHeight, kNearClip, kFarClip);
	camera_->TransferMatrix();
}

void CameraController::Update() {
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	Vector3 targetVelocity = target_->GetVelocity();

	// 最後に入力された進行方向を保持し、停止中も前方を広く映す
	if (std::abs(targetVelocity.x) > kDirectionThreshold) {
		facingDirection_ = targetVelocity.x > 0.0f ? 1.0f : -1.0f;
	}
	const float desiredLookAhead = facingDirection_ * kLookAheadDistance;
	lookAheadOffsetX_ = std::lerp(lookAheadOffsetX_, desiredLookAhead, kLookAheadInterpolationRate);

	targetPosition_.x = targetWorldTransform.translation_.x + targetOffset_.x + lookAheadOffsetX_ + targetVelocity.x * kVelocityBias;
	targetPosition_.y = CalculateCameraY(targetWorldTransform.translation_.y) + targetOffset_.y;
	targetPosition_.z = targetWorldTransform.translation_.z + targetOffset_.z + targetVelocity.z * kVelocityBias;

	camera_->translation_ = calc::Lerp(camera_->translation_, targetPosition_, kInterpolationRate);

	camera_->translation_.x = std::max(camera_->translation_.x, targetWorldTransform.translation_.x + kMargin.left);
	camera_->translation_.x = std::min(camera_->translation_.x, targetWorldTransform.translation_.x + kMargin.right);
	//camera_->translation_.y = std::max(camera_->translation_.y, targetWorldTransform.translation_.y + kMargin.bottom);
	//camera_->translation_.y = std::min(camera_->translation_.y, targetWorldTransform.translation_.y + kMargin.top);

	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);

	camera_->UpdateMatrix();
	ApplyOrthographicProjection();
}
