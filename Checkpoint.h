#pragma once
#include "KamataEngine.h"

// 中間ポイントの旗（通過すると、ミスした時にここから再開できる）
// 屋上のポールに付いたネオン看板の旗。通過前はネオンが消えていて、通過すると水色に光る
class Checkpoint {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position, bool activated);
	// 風で揺れる旗のモデルを設定する（設定しない時は Initialize のモデル全体を暗く・明るくする）
	void SetFlagModel(KamataEngine::Model* flagModel);
	void Update();
	void Draw();

	// プレイヤーのX座標が旗を越えたら通過とする
	bool IsPassedBy(const KamataEngine::Vector3& playerPosition) const;

	void Activate();
	bool IsActivated() const { return isActivated_; }

	const KamataEngine::Vector3& GetPosition() const { return worldTransform_.translation_; }

private:
	// 未通過の時の暗い色と、通過後の色
	static inline const KamataEngine::Vector4 kInactiveColor = {0.3f, 0.3f, 0.36f, 1.0f};
	static inline const KamataEngine::Vector4 kActiveColor = {1.0f, 1.0f, 1.0f, 1.0f};
	// 通過時に旗がはずむ演出
	static inline const float kPopDuration = 0.4f;
	static inline const float kPopScale = 0.35f;
	// 旗をポールに付ける位置（checkpoint_flag.obj の原点）
	static inline const KamataEngine::Vector3 kFlagPivot = {0.035f, 1.22f, 0.0f};
	// 旗が風で揺れる角度と速さ
	static inline const float kFlagWaveAngle = 0.3f;
	static inline const float kFlagWaveSpeed = 2.2f;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::ObjectColor objectColor_;

	KamataEngine::Model* flagModel_ = nullptr;
	KamataEngine::WorldTransform flagTransform_;
	float waveTimer_ = 0.0f;

	bool isActivated_ = false;
	float popTimer_ = 0.0f;
};
