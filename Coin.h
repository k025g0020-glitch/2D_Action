#pragma once
#include "CollisionMapInfo.h"
#include "KamataEngine.h"

// ステージに置かれたコイン
class Coin {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	AABB GetAABB() const;

	// コインを取る（取った後は少し跳ねてから消える）
	void Collect();
	// まだ取れる状態か
	bool IsAvailable() const { return state_ == State::kIdle; }

private:
	enum class State {
		kIdle,      // 置かれている
		kCollected, // 取られて跳ねている
		kGone,      // 消えた
	};

	static inline const float kRotateSpeed = 0.06f;
	static inline const float kCollectedRotateSpeed = 0.5f;
	// 取られた時に跳ねる初速と重力
	static inline const float kCollectedJumpSpeed = 0.2f;
	static inline const float kCollectedGravity = 0.02f;
	static inline const float kCollectedDuration = 0.35f;
	// 当たり判定の大きさ
	static inline const float kSize = 0.6f;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	State state_ = State::kIdle;
	float velocityY_ = 0.0f;
	float collectedTimer_ = 0.0f;
};
