#include "GoalStar.h"
#include "calc.h"
#include <cassert>
#include <cmath>
#include <numbers>

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

void GoalStar::SetBeamModel(Model* beamModel) {
	assert(beamModel);
	beamModel_ = beamModel;
	for (Beam* beam : {&beamCore_, &beamGlow_, &beamHaze_}) {
		beam->worldTransform.Initialize();
		beam->objectColor.Initialize();
	}
}

void GoalStar::UpdateBeam(Beam& beam, float width, const Vector4& color, float alpha) {
	WorldTransform& transform = beam.worldTransform;
	// 星の位置から上へ伸ばす（上下のふわふわには付いていかない）
	transform.translation_ = {basePosition_.x, basePosition_.y, kBeamZ};
	transform.scale_ = {width, kBeamHeight, 0.1f};
	transform.matWorld_ = calc::MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);
	transform.TransferMatrix();
	beam.objectColor.SetColor({color.x, color.y, color.z, alpha});
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

	if (beamModel_) {
		constexpr float kDeltaTime = 1.0f / 60.0f;
		beamTimer_ += kDeltaTime;
		// ゆっくり明滅する
		const float pulse = 1.0f + std::sin(beamTimer_ * kBeamPulseSpeed) * kBeamPulseAmount;
		float widthRate = 1.0f;
		float alphaRate = pulse;
		if (isCollected_) {
			// 取った時：一気に太く明るくなってから消えていく
			collectedTimer_ += kDeltaTime;
			const float t = (std::min)(collectedTimer_ / kBeamFlashDuration, 1.0f);
			widthRate = 1.0f + (kBeamFlashWidth - 1.0f) * std::sin(t * std::numbers::pi_v<float> * 0.5f);
			alphaRate = (1.0f - t) * 1.6f;
		}
		UpdateBeam(beamCore_, kBeamCoreWidth * widthRate, kBeamCoreColor, kBeamCoreColor.w * alphaRate);
		UpdateBeam(beamGlow_, kBeamGlowWidth * widthRate, kBeamGlowColor, kBeamGlowColor.w * alphaRate);
		UpdateBeam(beamHaze_, kBeamHazeWidth * widthRate, kBeamHazeColor, kBeamHazeColor.w * alphaRate);
	}
}

void GoalStar::Draw() {
	model_->Draw(worldTransform_, *camera_);

	// 半透明の柱は星の後に描く（外側から もや → 光 → 芯の順）
	if (beamModel_) {
		beamModel_->Draw(beamHaze_.worldTransform, *camera_, &beamHaze_.objectColor);
		beamModel_->Draw(beamGlow_.worldTransform, *camera_, &beamGlow_.objectColor);
		beamModel_->Draw(beamCore_.worldTransform, *camera_, &beamCore_.objectColor);
	}
}

AABB GoalStar::GetAABB() const {
	const Vector3& pos = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {pos.x - kWidth / 2.0f, pos.y - kHeight / 2.0f, pos.z - kWidth / 2.0f};
	aabb.max = {pos.x + kWidth / 2.0f, pos.y + kHeight / 2.0f, pos.z + kWidth / 2.0f};
	return aabb;
}

void GoalStar::Collect() { isCollected_ = true; }
