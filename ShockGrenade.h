#pragma once
#include "CollisionMapInfo.h"
#include "KamataEngine.h"

class MapChipField;

// 警備ロボットが投げる電撃グレネード
// 投げた時のプレイヤーの位置めがけて弧を描いて飛び、プレイヤーかブロックに当たると電撃をまき散らして消える
class ShockGrenade {
public:
	// origin から target に向かって投げる
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, MapChipField* mapChipField, const KamataEngine::Vector3& origin, const KamataEngine::Vector3& target);
	void Update();
	void Draw();

	AABB GetAABB() const;
	const KamataEngine::Vector3& GetPosition() const { return worldTransform_.translation_; }

	// 当たったので爆発させる
	void Explode() { isExploded_ = true; }
	// 爆発した（このフレームで消える。爆発の演出を出す）
	bool IsExploded() const { return isExploded_; }
	// 画面の下に落ちた・飛びすぎたので、演出なしで消す
	bool IsGone() const { return isGone_; }

private:
	// 重力（1フレームあたりの速度の変化）
	static inline const float kGravity = 0.012f;
	// 横方向の速さの目安（1フレームあたり）。遠いほど長く飛ぶ
	static inline const float kHorizontalSpeed = 0.15f;
	// 飛んでいる時間の最短・最長（フレーム）。近くても山なりに、遠くても速すぎないように
	static inline const float kMinFlightFrames = 36.0f;
	static inline const float kMaxFlightFrames = 70.0f;
	// 飛びながら回る速さ
	static inline const float kSpinSpeed = 0.25f;
	// 当たり判定の大きさ
	static inline const float kSize = 0.5f;
	// これより下に落ちたら消す
	static inline const float kFallOutY = -2.0f;
	// これより長く飛んだら消す<秒>
	static inline const float kLifeTime = 5.0f;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;

	KamataEngine::Vector3 velocity_ = {};
	float lifeTimer_ = 0.0f;
	bool isExploded_ = false;
	bool isGone_ = false;
};
