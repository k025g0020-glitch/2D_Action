#pragma once
#include "KamataEngine.h"
#include <vector>

// 砂ぼこり・キラキラ・紙吹雪などの小さなパーティクルをまとめて管理する
// 決まった数の粒を使い回し、空いている粒から順に使う
class ParticleSystem {
public:
	// sphereModel : 砂ぼこりなど丸い粒に使うモデル
	// starModel   : キラキラに使う星のモデル
	void Initialize(KamataEngine::Model* sphereModel, KamataEngine::Model* starModel, KamataEngine::Camera* camera);
	void Update();
	void Draw();

	// ---- よく使う演出 ----
	// ジャンプした時の足元の砂ぼこり（stage は3段ジャンプの段数）
	void EmitJumpDust(const KamataEngine::Vector3& feet, uint32_t stage);
	// 着地した時の砂ぼこり（fallSpeed は着地直前の落下の速さ）
	void EmitLandingDust(const KamataEngine::Vector3& feet, float fallSpeed);
	// 走っている時・ブレーキをかけた時に足元から出る砂ぼこり（direction は飛ばす向き -1 / +1）
	void EmitRunDust(const KamataEngine::Vector3& feet, float direction);
	// キラキラが四方に飛び散る
	void EmitSparkles(const KamataEngine::Vector3& position, const KamataEngine::Vector4& color, uint32_t count, float speed);
	// 3段ジャンプの3段目で出る星の輪
	void EmitStarRing(const KamataEngine::Vector3& position);
	// 敵を踏んだ時の衝撃
	void EmitStompImpact(const KamataEngine::Vector3& position);
	// 中間ポイント通過の紙吹雪
	void EmitConfetti(const KamataEngine::Vector3& position);
	// 星のまわりで小さく光る
	void EmitTwinkle(const KamataEngine::Vector3& center, float radius);

private:
	struct Particle {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor objectColor;
		KamataEngine::Model* model = nullptr;
		KamataEngine::Vector3 velocity = {};
		KamataEngine::Vector4 color = {};
		float startScale = 0.0f;
		float endScale = 0.0f;
		float gravity = 0.0f;
		// 毎フレーム速度に掛ける割合（1未満で減速）
		float drag = 1.0f;
		float spin = 0.0f;
		float life = 0.0f;
		float age = 0.0f;
		bool active = false;
	};

	struct EmitDesc {
		KamataEngine::Model* model = nullptr;
		KamataEngine::Vector3 position = {};
		// 位置のばらつき
		KamataEngine::Vector3 positionRandom = {};
		KamataEngine::Vector3 velocity = {};
		// 速度のばらつき
		KamataEngine::Vector3 velocityRandom = {};
		KamataEngine::Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		float startScale = 0.2f;
		float endScale = 0.0f;
		float gravity = 0.0f;
		float drag = 1.0f;
		float spin = 0.0f;
		float life = 0.5f;
	};

	// 同時に出せる粒の最大数
	static inline const uint32_t kMaxParticles = 384;

	void Emit(const EmitDesc& desc);
	Particle* FindFreeParticle();

	std::vector<Particle> particles_;
	uint32_t nextIndex_ = 0;

	KamataEngine::Model* sphereModel_ = nullptr;
	KamataEngine::Model* starModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
};
