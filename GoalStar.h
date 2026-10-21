#pragma once
#include "CollisionMapInfo.h"
#include "KamataEngine.h"

// ゴール地点に置かれる獲得できる星
class GoalStar {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	AABB GetAABB() const;

	// 星を獲得する
	void Collect();
	bool IsCollected() const { return isCollected_; }

private:
	// 回転速度（ラジアン/フレーム）
	static inline const float kRotateSpeed = 0.05f;
	// 獲得後の回転速度
	static inline const float kCollectedRotateSpeed = 0.4f;
	// 上下にふわふわ動く幅と速度
	static inline const float kFloatAmplitude = 0.15f;
	static inline const float kFloatSpeed = 0.05f;
	// 獲得演出の上昇速度と拡大率
	static inline const float kCollectedRiseSpeed = 0.06f;
	static inline const float kCollectedScaleSpeed = 0.02f;
	static inline const float kCollectedMaxScale = 1.8f;
	// 当たり判定の大きさ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::Vector3 basePosition_ = {};
	float floatTimer_ = 0.0f;

	bool isCollected_ = false;
};
