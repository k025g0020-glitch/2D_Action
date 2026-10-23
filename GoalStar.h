#pragma once
#include "CollisionMapInfo.h"
#include "KamataEngine.h"

// ゴール地点に置かれる獲得できる星
// 星から空へ光の柱が伸び、離れた所からでもゴールの位置が分かる
class GoalStar {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	// 光の柱のモデル（白い箱）を設定する。設定しない時は柱を出さない
	void SetBeamModel(KamataEngine::Model* beamModel);
	void Update();
	void Draw();

	AABB GetAABB() const;

	// 星を獲得する
	void Collect();
	bool IsCollected() const { return isCollected_; }

	// 光の柱の根元（星が浮いている中心）と、柱の半分の幅
	const KamataEngine::Vector3& GetBasePosition() const { return basePosition_; }
	float GetBeamHalfWidth() const { return kBeamGlowWidth * 0.5f; }

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

	// ---- 光の柱 ----
	// 細く明るい芯、外側のやわらかい光、さらに外側の薄いもやの3本を重ねる
	struct Beam {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor objectColor;
	};
	void UpdateBeam(Beam& beam, float width, const KamataEngine::Vector4& color, float alpha);

	// 柱の高さ（ステージの上の端より高く）
	static inline const float kBeamHeight = 30.0f;
	// 芯・外側の光・もやの幅
	static inline const float kBeamCoreWidth = 0.24f;
	static inline const float kBeamGlowWidth = 0.8f;
	static inline const float kBeamHazeWidth = 2.2f;
	// 芯・外側の光・もやの色（透明度も含む）
	static inline const KamataEngine::Vector4 kBeamCoreColor = {1.0f, 0.96f, 0.8f, 0.7f};
	static inline const KamataEngine::Vector4 kBeamGlowColor = {1.0f, 0.8f, 0.35f, 0.28f};
	static inline const KamataEngine::Vector4 kBeamHazeColor = {1.0f, 0.75f, 0.35f, 0.09f};
	// 足場より少し奥に置く（足場の後ろから光が立ち上り、キャラクターは光の前に見える）
	static inline const float kBeamZ = 0.6f;
	// 明滅の速さと強さ
	static inline const float kBeamPulseSpeed = 3.0f;
	static inline const float kBeamPulseAmount = 0.25f;
	// 取った時に柱が太くなって消えるまでの時間と、最大の太さの倍率
	static inline const float kBeamFlashDuration = 1.0f;
	static inline const float kBeamFlashWidth = 3.0f;

	KamataEngine::Model* beamModel_ = nullptr;
	Beam beamCore_;
	Beam beamGlow_;
	Beam beamHaze_;
	float beamTimer_ = 0.0f;
	float collectedTimer_ = 0.0f;

	bool isCollected_ = false;
};
