#define NOMINMAX
#include "SmokeParticles.h"
#include "calc.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include "Random.h"

using namespace KamataEngine;

void SmokeParticles::Initialize(Model* model, Camera* camera) {
	assert(model);

	model_ = model;
	camera_ = camera;

	for (Particle& particle : particles_) {
		particle.worldTransform.Initialize();
	}

	objectColor_.Initialize();

	isActive_ = false;
	counter_ = 0.0f;
}

void SmokeParticles::Start(const Vector3& position) {
	const float angleUnit = 2.0f * std::numbers::pi_v<float> / static_cast<float>(kNumParticles);

	for (uint32_t i = 0; i < kNumParticles; ++i) {
		Particle& particle = particles_[i];

		// 円状に広がるように、少しばらつきを持たせた方向へ飛ばす
		const float angle = angleUnit * static_cast<float>(i) + RandomRange(-0.3f, 0.3f);
		const float speed = kSpeed * RandomRange(0.6f, 1.2f);

		particle.velocity = {std::cos(angle) * speed, std::sin(angle) * speed * 0.7f, 0.0f};
		particle.scaleRate = RandomRange(0.7f, 1.2f);

		particle.worldTransform.translation_ = {
		    position.x + RandomRange(-0.1f, 0.1f),
		    position.y + RandomRange(-0.1f, 0.1f),
		    // 敵より手前に出して敵の消える瞬間を隠す
		    position.z - 0.3f,
		};
		const float scale = kStartScale * particle.scaleRate;
		particle.worldTransform.scale_ = {scale, scale, scale};
		particle.worldTransform.rotation_ = {0.0f, 0.0f, RandomRange(0.0f, std::numbers::pi_v<float>)};
	}

	counter_ = 0.0f;
	color_.w = kStartAlpha;
	objectColor_.SetColor(color_);
	isActive_ = true;
}

void SmokeParticles::Update() {
	if (!isActive_) {
		return;
	}

	counter_ += 1.0f / 60.0f;
	const float t = std::clamp(counter_ / kDuration, 0.0f, 1.0f);

	// 最初に勢いよく膨らみ、だんだんゆっくりになる
	const float easeOut = 1.0f - (1.0f - t) * (1.0f - t);

	for (Particle& particle : particles_) {
		// 広がる速度は徐々に減衰し、全体はゆっくり上昇する
		particle.velocity.x *= kSpeedAttenuation;
		particle.velocity.y *= kSpeedAttenuation;

		particle.worldTransform.translation_.x += particle.velocity.x;
		particle.worldTransform.translation_.y += particle.velocity.y + kRiseSpeed;

		const float scale = std::lerp(kStartScale, kMaxScale, easeOut) * particle.scaleRate;
		particle.worldTransform.scale_ = {scale, scale, scale};

		WorldTransform& worldTransform = particle.worldTransform;
		worldTransform.matWorld_ = calc::MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
		worldTransform.TransferMatrix();
	}

	// 後半にかけて薄くなって消える
	color_.w = kStartAlpha * (1.0f - t * t);
	objectColor_.SetColor(color_);

	if (counter_ >= kDuration) {
		isActive_ = false;
	}
}

void SmokeParticles::Draw() {
	if (!isActive_) {
		return;
	}

	for (Particle& particle : particles_) {
		model_->Draw(particle.worldTransform, *camera_, &objectColor_);
	}
}
