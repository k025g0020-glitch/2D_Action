#pragma once
#include "KamataEngine.h"
#include <array>

// プレイヤーがミスした時の演出
// 1. 一瞬まぶしく光り、体がネオンの水色・マゼンタ・黒いコートの色の欠片に砕けて飛び散る
// 2. 水色とマゼンタのネオンの輪が広がる
// 3. 細かい光の粒が、データのように上へ昇って消える
class DeathParticles {
public:
	// shardModel : 欠片・光・粒に使う白い立方体、ringModel : 広がる輪（どちらも ObjectColor で色を付ける）
	void Initialize(KamataEngine::Model* shardModel, KamataEngine::Model* ringModel, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	bool IsFinished() const { return isFinished_; }

private:
	// 動く粒（欠片・昇る光の粒）
	struct Particle {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor objectColor;
		KamataEngine::Vector3 velocity = {};
		KamataEngine::Vector3 spin = {};
		KamataEngine::Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		float scale = 0.2f;
		// 出てくるまでの時間と、出てから消えるまでの時間<秒>
		float delay = 0.0f;
		float life = 1.0f;
		// 今表示しているか（出てくる前と消えた後は描かない）
		bool isVisible = false;
	};
	// 広がる輪・光
	struct Flash {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor objectColor;
		KamataEngine::Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		float delay = 0.0f;
		float duration = 0.3f;
		float startScale = 0.0f;
		float endScale = 1.0f;
		bool isVisible = false;
	};

	void UpdateParticle(Particle& particle);
	void UpdateFlash(Flash& flash);

	// 演出全体の長さ<秒>（これが過ぎたら終わり）
	static inline const float kDuration = 1.4f;

	// 体が砕けた欠片
	static inline const uint32_t kNumShards = 40;
	// 欠片が飛び出す速さと、上向きに足す速さ、重力
	static inline const float kShardMinSpeed = 0.05f;
	static inline const float kShardMaxSpeed = 0.17f;
	static inline const float kShardUpward = 0.05f;
	static inline const float kShardGravity = 0.005f;
	static inline const float kShardDrag = 0.97f;

	// データのように昇っていく光の粒
	static inline const uint32_t kNumBits = 24;
	static inline const float kBitRiseSpeed = 0.035f;

	// 色（プレイヤーのモデルと同じ色）
	static inline const KamataEngine::Vector4 kNeon = {0.16f, 0.92f, 1.0f, 1.0f};
	static inline const KamataEngine::Vector4 kVisor = {0.6f, 1.0f, 1.0f, 1.0f};
	static inline const KamataEngine::Vector4 kScarf = {0.95f, 0.2f, 0.55f, 1.0f};
	static inline const KamataEngine::Vector4 kCoat = {0.16f, 0.17f, 0.24f, 1.0f};

	std::array<Particle, kNumShards> shards_;
	std::array<Particle, kNumBits> bits_;
	// まぶしい光と、水色・マゼンタの輪
	std::array<Flash, 3> flashes_;

	KamataEngine::Model* shardModel_ = nullptr;
	KamataEngine::Model* ringModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	float counter_ = 0.0f;
	bool isFinished_ = false;
};
