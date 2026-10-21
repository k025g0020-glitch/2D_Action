#pragma once
#include "KamataEngine.h"

// 中間ポイントの旗（通過すると、ミスした時にここから再開できる）
class Checkpoint {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, bool activated);
	void Update();
	void Draw();

	// プレイヤーのX座標が旗を越えたら通過とする
	bool IsPassedBy(const KamataEngine::Vector3& playerPosition) const;

	void Activate();
	bool IsActivated() const { return isActivated_; }

	const KamataEngine::Vector3& GetPosition() const { return worldTransform_.translation_; }

private:
	// 未通過の時の暗い色と、通過後の色
	static inline const KamataEngine::Vector4 kInactiveColor = {0.35f, 0.35f, 0.4f, 1.0f};
	static inline const KamataEngine::Vector4 kActiveColor = {1.0f, 1.0f, 1.0f, 1.0f};
	// 通過時に旗がはずむ演出
	static inline const float kPopDuration = 0.4f;
	static inline const float kPopScale = 0.35f;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::ObjectColor objectColor_;

	bool isActivated_ = false;
	float popTimer_ = 0.0f;
};
