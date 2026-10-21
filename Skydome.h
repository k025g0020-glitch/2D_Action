#pragma once
#include "KamataEngine.h"


class Skydome {
public:
	void Initialize(KamataEngine::Model* model);
	void Update();
	void Draw(KamataEngine::Camera& camera);

	// カメラに合わせて横に動かす（ステージが広くても天球の外に出ないようにする）
	void Follow(const KamataEngine::Vector3& cameraPosition);

private:
	// カメラの横移動に追従する割合（1未満にすると背景がゆっくり流れて奥行きが出る）
	static inline const float kFollowRate = 0.9f;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
};