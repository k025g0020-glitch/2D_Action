#pragma once
#include "KamataEngine.h"
#include <array>

// 敵を倒した時に出る煙のパーティクル
class SmokeParticles {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera);
	// 指定位置から煙を発生させる
	void Start(const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	bool IsActive() const { return isActive_; }

private:
	static inline const uint32_t kNumParticles = 14;
	// 煙が消えるまでの時間（秒）
	static inline const float kDuration = 0.6f;
	// 初速と減衰率
	static inline const float kSpeed = 0.1f;
	static inline const float kSpeedAttenuation = 0.9f;
	// 煙全体が上へ昇る速さ
	static inline const float kRiseSpeed = 0.01f;
	// 大きさ（発生時 → 最大）
	static inline const float kStartScale = 0.5f;
	static inline const float kMaxScale = 1.4f;
	// 不透明度の初期値
	static inline const float kStartAlpha = 0.85f;

	struct Particle {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::Vector3 velocity;
		// 粒ごとの大きさのばらつき
		float scaleRate;
	};

	std::array<Particle, kNumParticles> particles_;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	bool isActive_ = false;
	float counter_ = 0.0f;

	// 色変更オブジェクト
	KamataEngine::ObjectColor objectColor_;
	KamataEngine::Vector4 color_ = {0.9f, 0.9f, 0.9f, 1.0f};
};
