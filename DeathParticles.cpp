#include "DeathParticles.h"
#include "Random.h"
#include "calc.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void DeathParticles::Initialize(Model* shardModel, Model* ringModel, Camera* camera, const Vector3& position) {
	assert(shardModel && ringModel);
	shardModel_ = shardModel;
	ringModel_ = ringModel;
	camera_ = camera;
	counter_ = 0.0f;
	isFinished_ = false;

	// 体が砕けた欠片：体（1.6 マス四方）のあちこちから、外側へ飛び散る
	for (uint32_t i = 0; i < kNumShards; ++i) {
		Particle& shard = shards_[i];
		const Vector3 offset = {RandomRange(-0.6f, 0.6f), RandomRange(-0.7f, 0.7f), 0.0f};
		const float angle = std::atan2(offset.y, offset.x) + RandomRange(-0.4f, 0.4f);
		const float speed = RandomRange(kShardMinSpeed, kShardMaxSpeed);
		shard.worldTransform.Initialize();
		shard.worldTransform.translation_ = {position.x + offset.x, position.y + offset.y, position.z - 0.3f};
		shard.velocity = {std::cos(angle) * speed, std::sin(angle) * speed + kShardUpward, 0.0f};
		shard.spin = {RandomRange(-0.3f, 0.3f), RandomRange(-0.3f, 0.3f), RandomRange(-0.3f, 0.3f)};
		shard.scale = RandomRange(0.12f, 0.32f);
		shard.delay = 0.0f;
		shard.life = RandomRange(0.6f, 1.1f);
		// 半分は水色のネオン、残りをバイザー・マフラー・コートの色に分ける
		const float pick = RandomRange(0.0f, 1.0f);
		shard.color = (pick < 0.5f) ? kNeon : (pick < 0.65f) ? kVisor : (pick < 0.82f) ? kScarf : kCoat;
		shard.objectColor.Initialize();
	}

	// 昇っていく光の粒：少し遅れて、体のあった所から湧き出す
	for (uint32_t i = 0; i < kNumBits; ++i) {
		Particle& bit = bits_[i];
		bit.worldTransform.Initialize();
		bit.worldTransform.translation_ = {position.x + RandomRange(-0.7f, 0.7f), position.y + RandomRange(-0.8f, 0.6f), position.z - 0.35f};
		bit.velocity = {RandomRange(-0.005f, 0.005f), kBitRiseSpeed * RandomRange(0.6f, 1.3f), 0.0f};
		bit.spin = {};
		bit.scale = RandomRange(0.07f, 0.13f);
		bit.delay = RandomRange(0.15f, 0.55f);
		bit.life = RandomRange(0.5f, 0.8f);
		bit.color = (i % 4 == 0) ? kScarf : kVisor;
		bit.objectColor.Initialize();
	}

	// まぶしい光（すぐ消える）と、水色・マゼンタの順に広がる輪
	struct FlashSetting {
		Vector4 color;
		float delay, duration, startScale, endScale;
	};
	const FlashSetting kFlashes[] = {
	    {{0.85f, 1.0f, 1.0f, 1.0f}, 0.0f, 0.18f, 1.2f, 2.2f},
	    {kNeon, 0.0f, 0.4f, 0.4f, 3.2f},
	    {kScarf, 0.1f, 0.45f, 0.3f, 2.4f},
	};
	for (size_t i = 0; i < flashes_.size(); ++i) {
		Flash& flash = flashes_[i];
		flash.color = kFlashes[i].color;
		flash.delay = kFlashes[i].delay;
		flash.duration = kFlashes[i].duration;
		flash.startScale = kFlashes[i].startScale;
		flash.endScale = kFlashes[i].endScale;
		flash.worldTransform.Initialize();
		flash.worldTransform.translation_ = {position.x, position.y, position.z - 0.4f};
		flash.objectColor.Initialize();
	}
	Update();
}

void DeathParticles::UpdateParticle(Particle& particle) {
	const float time = counter_ - particle.delay;
	particle.isVisible = time >= 0.0f && time <= particle.life;
	if (!particle.isVisible) {
		return;
	}
	WorldTransform& worldTransform = particle.worldTransform;
	worldTransform.translation_ = calc::Add(worldTransform.translation_, particle.velocity);
	worldTransform.rotation_ = calc::Add(worldTransform.rotation_, particle.spin);

	// だんだん小さくなり、終わりの方で薄くなって消える
	const float t = time / particle.life;
	const float scale = particle.scale * (1.0f - 0.6f * t);
	worldTransform.scale_ = {scale, scale, scale};
	Vector4 color = particle.color;
	color.w = std::clamp((1.0f - t) / 0.4f, 0.0f, 1.0f);
	particle.objectColor.SetColor(color);

	worldTransform.matWorld_ = calc::MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
	worldTransform.TransferMatrix();
}

void DeathParticles::UpdateFlash(Flash& flash) {
	const float t = (counter_ - flash.delay) / flash.duration;
	flash.isVisible = t >= 0.0f && t <= 1.0f;
	if (!flash.isVisible) {
		return;
	}
	// 素早く広がって、だんだんゆっくりに
	const float ease = 1.0f - (1.0f - t) * (1.0f - t);
	const float scale = std::lerp(flash.startScale, flash.endScale, ease);
	flash.worldTransform.scale_ = {scale, scale, scale};
	Vector4 color = flash.color;
	color.w = 1.0f - t;
	flash.objectColor.SetColor(color);
	flash.worldTransform.matWorld_ = calc::MakeAffineMatrix(flash.worldTransform.scale_, flash.worldTransform.rotation_, flash.worldTransform.translation_);
	flash.worldTransform.TransferMatrix();
}

void DeathParticles::Update() {
	if (isFinished_) {
		return;
	}

	for (Particle& shard : shards_) {
		// 空気の抵抗で少しずつ遅くなり、重力で落ちていく
		if (counter_ >= shard.delay) {
			shard.velocity.x *= kShardDrag;
			shard.velocity.y = shard.velocity.y * kShardDrag - kShardGravity;
		}
		UpdateParticle(shard);
	}
	for (Particle& bit : bits_) {
		UpdateParticle(bit);
	}
	for (Flash& flash : flashes_) {
		UpdateFlash(flash);
	}

	counter_ += 1.0f / 60.0f;
	if (counter_ >= kDuration) {
		isFinished_ = true;
	}
}

void DeathParticles::Draw() {
	if (isFinished_) {
		return;
	}
	// まぶしい光（立方体）→ 輪 → 欠片 → 光の粒 の順に描く（Update で位置を決めたものだけ）
	for (size_t i = 0; i < flashes_.size(); ++i) {
		Flash& flash = flashes_[i];
		if (flash.isVisible) {
			(i == 0 ? shardModel_ : ringModel_)->Draw(flash.worldTransform, *camera_, &flash.objectColor);
		}
	}
	for (Particle& shard : shards_) {
		if (shard.isVisible) {
			shardModel_->Draw(shard.worldTransform, *camera_, &shard.objectColor);
		}
	}
	for (Particle& bit : bits_) {
		if (bit.isVisible) {
			shardModel_->Draw(bit.worldTransform, *camera_, &bit.objectColor);
		}
	}
}
