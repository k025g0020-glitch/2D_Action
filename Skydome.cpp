#include "Skydome.h"
#include "calc.h"

using namespace KamataEngine;


void Skydome::Initialize(KamataEngine::Model* model) {
	model_ = model;
	worldTransform_.Initialize();
}

void Skydome::Update() {
	worldTransform_.matWorld_ = calc::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Skydome::Follow(const KamataEngine::Vector3& cameraPosition) { worldTransform_.translation_.x = cameraPosition.x * kFollowRate; }

void Skydome::Draw(KamataEngine::Camera& camera) { 
if (model_) {
		model_->Draw(worldTransform_, camera);
	}
}