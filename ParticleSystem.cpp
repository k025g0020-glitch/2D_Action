#define NOMINMAX
#include "ParticleSystem.h"
#include "Random.h"
#include "calc.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <numbers>

using namespace KamataEngine;

namespace {
// 砂ぼこりの色
const Vector4 kDustColor = {0.92f, 0.88f, 0.8f, 0.85f};
// 紙吹雪の色
const Vector4 kConfettiColors[] = {
    {1.0f, 0.35f, 0.3f, 1.0f},
    {1.0f, 0.85f, 0.25f, 1.0f},
    {0.35f, 0.8f, 1.0f, 1.0f},
    {0.45f, 0.95f, 0.45f, 1.0f},
    {0.95f, 0.5f, 1.0f, 1.0f},
};
} // namespace

void ParticleSystem::Initialize(Model* sphereModel, Model* starModel, Camera* camera) {
	assert(sphereModel);
	assert(starModel);

	sphereModel_ = sphereModel;
	starModel_ = starModel;
	camera_ = camera;

	for (Particle& particle : particles_) {
		particle.worldTransform.Initialize();
		particle.objectColor.Initialize();
		particle.active = false;
	}
	nextIndex_ = 0;
}

ParticleSystem::Particle* ParticleSystem::FindFreeParticle() {
	// 空いている粒を探す。全部使っていたら一番古いものから上書きする
	for (uint32_t i = 0; i < kMaxParticles; ++i) {
		Particle& particle = particles_[(nextIndex_ + i) % kMaxParticles];
		if (!particle.active) {
			nextIndex_ = (nextIndex_ + i + 1) % kMaxParticles;
			return &particle;
		}
	}
	Particle& oldest = particles_[nextIndex_];
	nextIndex_ = (nextIndex_ + 1) % kMaxParticles;
	return &oldest;
}

void ParticleSystem::Emit(const EmitDesc& desc) {
	Particle* particle = FindFreeParticle();

	particle->model = desc.model;
	particle->worldTransform.translation_ = {
	    desc.position.x + RandomRange(-desc.positionRandom.x, desc.positionRandom.x),
	    desc.position.y + RandomRange(-desc.positionRandom.y, desc.positionRandom.y),
	    desc.position.z + RandomRange(-desc.positionRandom.z, desc.positionRandom.z),
	};
	particle->velocity = {
	    desc.velocity.x + RandomRange(-desc.velocityRandom.x, desc.velocityRandom.x),
	    desc.velocity.y + RandomRange(-desc.velocityRandom.y, desc.velocityRandom.y),
	    desc.velocity.z + RandomRange(-desc.velocityRandom.z, desc.velocityRandom.z),
	};
	particle->worldTransform.rotation_ = {0.0f, 0.0f, RandomRange(0.0f, 2.0f * std::numbers::pi_v<float>)};
	particle->worldTransform.scale_ = {desc.startScale, desc.startScale, desc.startScale};
	particle->color = desc.color;
	particle->startScale = desc.startScale;
	particle->endScale = desc.endScale;
	particle->gravity = desc.gravity;
	particle->drag = desc.drag;
	particle->spin = desc.spin * (RandomRange(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f);
	particle->life = desc.life * RandomRange(0.8f, 1.2f);
	particle->age = 0.0f;
	particle->active = true;
	particle->objectColor.SetColor(desc.color);
}

void ParticleSystem::Update() {
	constexpr float kDeltaTime = 1.0f / 60.0f;

	for (Particle& particle : particles_) {
		if (!particle.active) {
			continue;
		}

		particle.age += kDeltaTime;
		if (particle.age >= particle.life) {
			particle.active = false;
			continue;
		}
		const float t = particle.age / particle.life;

		particle.velocity.y -= particle.gravity;
		particle.velocity.x *= particle.drag;
		particle.velocity.y *= particle.drag;
		particle.velocity.z *= particle.drag;

		WorldTransform& worldTransform = particle.worldTransform;
		worldTransform.translation_.x += particle.velocity.x;
		worldTransform.translation_.y += particle.velocity.y;
		worldTransform.translation_.z += particle.velocity.z;
		worldTransform.rotation_.z += particle.spin;

		const float scale = std::lerp(particle.startScale, particle.endScale, t);
		worldTransform.scale_ = {scale, scale, scale};

		// 後半にかけて薄くなる
		Vector4 color = particle.color;
		color.w *= 1.0f - t * t;
		particle.objectColor.SetColor(color);

		worldTransform.matWorld_ = calc::MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
		worldTransform.TransferMatrix();
	}
}

void ParticleSystem::Draw() {
	for (Particle& particle : particles_) {
		if (particle.active) {
			particle.model->Draw(particle.worldTransform, *camera_, &particle.objectColor);
		}
	}
}

void ParticleSystem::EmitJumpDust(const Vector3& feet, uint32_t stage) {
	// 段数が上がるほど派手にする
	const uint32_t count = 4 + stage * 3;
	for (uint32_t i = 0; i < count; ++i) {
		const float direction = (i % 2 == 0) ? 1.0f : -1.0f;
		EmitDesc desc;
		desc.model = sphereModel_;
		desc.position = {feet.x, feet.y + 0.05f, feet.z - 0.2f};
		desc.positionRandom = {0.15f, 0.03f, 0.0f};
		desc.velocity = {direction * 0.06f, 0.015f, 0.0f};
		desc.velocityRandom = {0.03f, 0.015f, 0.0f};
		desc.color = kDustColor;
		desc.startScale = 0.18f;
		desc.endScale = 0.4f;
		desc.drag = 0.88f;
		desc.life = 0.35f;
		Emit(desc);
	}
}

void ParticleSystem::EmitLandingDust(const Vector3& feet, float fallSpeed) {
	// 高い所から落ちるほど多く、遠くまで広がる
	const float strength = std::clamp(fallSpeed / 0.5f, 0.0f, 1.0f);
	const uint32_t count = 4 + static_cast<uint32_t>(strength * 8.0f);
	for (uint32_t i = 0; i < count; ++i) {
		const float direction = (i % 2 == 0) ? 1.0f : -1.0f;
		EmitDesc desc;
		desc.model = sphereModel_;
		desc.position = {feet.x + direction * 0.2f, feet.y + 0.05f, feet.z - 0.2f};
		desc.positionRandom = {0.1f, 0.03f, 0.0f};
		desc.velocity = {direction * (0.05f + 0.08f * strength), 0.01f, 0.0f};
		desc.velocityRandom = {0.03f, 0.01f, 0.0f};
		desc.color = kDustColor;
		desc.startScale = 0.15f + 0.1f * strength;
		desc.endScale = 0.35f + 0.15f * strength;
		desc.drag = 0.86f;
		desc.life = 0.35f + 0.15f * strength;
		Emit(desc);
	}
}

void ParticleSystem::EmitRunDust(const Vector3& feet, float direction) {
	EmitDesc desc;
	desc.model = sphereModel_;
	desc.position = {feet.x, feet.y + 0.08f, feet.z - 0.2f};
	desc.positionRandom = {0.05f, 0.03f, 0.0f};
	desc.velocity = {direction * 0.03f, 0.02f, 0.0f};
	desc.velocityRandom = {0.01f, 0.01f, 0.0f};
	desc.color = kDustColor;
	desc.startScale = 0.15f;
	desc.endScale = 0.3f;
	desc.drag = 0.9f;
	desc.life = 0.3f;
	Emit(desc);
}

void ParticleSystem::EmitSparkles(const Vector3& position, const Vector4& color, uint32_t count, float speed) {
	const float angleUnit = 2.0f * std::numbers::pi_v<float> / static_cast<float>(count);
	for (uint32_t i = 0; i < count; ++i) {
		const float angle = angleUnit * static_cast<float>(i) + RandomRange(-0.3f, 0.3f);
		const float s = speed * RandomRange(0.6f, 1.2f);
		EmitDesc desc;
		desc.model = starModel_;
		desc.position = {position.x, position.y, position.z - 0.3f};
		desc.velocity = {std::cos(angle) * s, std::sin(angle) * s, 0.0f};
		desc.color = color;
		desc.startScale = 0.25f;
		desc.endScale = 0.0f;
		desc.gravity = 0.004f;
		desc.drag = 0.92f;
		desc.spin = 0.2f;
		desc.life = 0.5f;
		Emit(desc);
	}
}

void ParticleSystem::EmitStarRing(const Vector3& position) {
	// 星が輪になって広がる
	constexpr uint32_t kCount = 12;
	const float angleUnit = 2.0f * std::numbers::pi_v<float> / static_cast<float>(kCount);
	for (uint32_t i = 0; i < kCount; ++i) {
		const float angle = angleUnit * static_cast<float>(i);
		EmitDesc desc;
		desc.model = starModel_;
		desc.position = {position.x, position.y, position.z - 0.3f};
		desc.velocity = {std::cos(angle) * 0.12f, std::sin(angle) * 0.12f, 0.0f};
		desc.color = {1.0f, 0.95f, 0.5f, 1.0f};
		desc.startScale = 0.35f;
		desc.endScale = 0.05f;
		desc.drag = 0.9f;
		desc.spin = 0.15f;
		desc.life = 0.6f;
		Emit(desc);
	}
}

void ParticleSystem::EmitStompImpact(const Vector3& position) {
	// 白い星がはじける
	EmitSparkles(position, {1.0f, 1.0f, 1.0f, 1.0f}, 8, 0.12f);
	// 横に広がる砂ぼこり
	for (uint32_t i = 0; i < 6; ++i) {
		const float direction = (i % 2 == 0) ? 1.0f : -1.0f;
		EmitDesc desc;
		desc.model = sphereModel_;
		desc.position = {position.x, position.y - 0.2f, position.z - 0.25f};
		desc.velocity = {direction * 0.09f, 0.02f, 0.0f};
		desc.velocityRandom = {0.03f, 0.02f, 0.0f};
		desc.color = {1.0f, 1.0f, 1.0f, 0.8f};
		desc.startScale = 0.15f;
		desc.endScale = 0.3f;
		desc.drag = 0.85f;
		desc.life = 0.3f;
		Emit(desc);
	}
}

void ParticleSystem::EmitConfetti(const Vector3& position) {
	constexpr uint32_t kCount = 28;
	for (uint32_t i = 0; i < kCount; ++i) {
		EmitDesc desc;
		desc.model = sphereModel_;
		desc.position = {position.x, position.y, position.z - 0.3f};
		desc.positionRandom = {0.2f, 0.1f, 0.0f};
		desc.velocity = {0.0f, 0.2f, 0.0f};
		desc.velocityRandom = {0.12f, 0.08f, 0.0f};
		desc.color = kConfettiColors[i % std::size(kConfettiColors)];
		desc.startScale = 0.14f;
		desc.endScale = 0.1f;
		desc.gravity = 0.012f;
		desc.drag = 0.96f;
		desc.spin = 0.3f;
		desc.life = 1.2f;
		Emit(desc);
	}
}

void ParticleSystem::EmitTwinkle(const Vector3& center, float radius) {
	EmitDesc desc;
	desc.model = starModel_;
	desc.position = {center.x, center.y, center.z - 0.3f};
	desc.positionRandom = {radius, radius, 0.0f};
	desc.velocity = {0.0f, 0.01f, 0.0f};
	desc.color = {1.0f, 1.0f, 0.8f, 1.0f};
	desc.startScale = 0.2f;
	desc.endScale = 0.0f;
	desc.spin = 0.1f;
	desc.life = 0.6f;
	Emit(desc);
}
