#pragma once
#include "KamataEngine.h"
#include "CollisionMapInfo.h"
#include <cassert>

class MapChipField;

class Enemy;

class Player {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, MapChipField* mapChipField, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	const KamataEngine::Vector3& GetTranslation() const { return worldTransform_.translation_; }
	void SetTranslation(const KamataEngine::Vector3& translation) { worldTransform_.translation_ = translation; }
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	KamataEngine::Vector3 GetWorldPosition();
	AABB GetAABB();

	void OnCollision(const Enemy* enemy);
	void BounceFromEnemy();

	bool IsDead() const { return isDead_; }
	// 穴への落下や時間切れでミスにする
	void Die() { isDead_ = true; }
	bool IsOnGround() const { return onGround_; }

	// 一定時間無敵にする（復活直後など）。無敵中は点滅し、敵に触れてもミスにならない
	void StartInvincible(float seconds) { invincibleTimer_ = seconds; }
	bool IsInvincible() const { return invincibleTimer_ > 0.0f; }

private:

	enum Corner {
		kRightBottom, // 右下
		kLeftBottom,  // 左下
		kRightTop,    // 右上
		kLeftTop,     // 左上
		kNumCorner    // 要素数
	};

	void Move();

	// 判定結果を反映して移動
	void ApplyMove(const CollisionMapInfo& info);

	// 天井に接触している場合の処理
	void OnCeilingCollision(const CollisionMapInfo& info);

	void OnWallCollision(const CollisionMapInfo& info);

	// 追接地状態の切り替え処理
	void OnGroundToggle(const CollisionMapInfo& info);

	// 角の座標計算関数
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

	// マップ衝突判定の主関数
	void MapCollision(CollisionMapInfo& info);

	// 各方向の衝突判定小分け関数
	void MapCollisionUp(CollisionMapInfo& info);
	void MapCollisionDown(CollisionMapInfo& info);
	void MapCollisionRight(CollisionMapInfo& info);
	void MapCollisionLeft(CollisionMapInfo& info);

	enum class LRDirection {
		kRight,
		kLeft,
	};

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::Vector3 velocity_ = {};

	MapChipField* mapChipField_ = nullptr;

	LRDirection lrDirection_ = LRDirection::kRight;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;
	// 旋回タイマー
	float turnTimer_ = 0.0f;
	// 旋回時間<秒>
	static inline const float kTimeTurn = 0.3f;

	// 地上での加速度（最高速まで約0.1秒）
	static inline const float kAcceleration = 0.03f;
	// 入力がない時の減速率（大きいほどピタッと止まる）
	static inline const float kAttenuation = 0.25f;
	// 進行方向と逆に入力した時のブレーキの強さ
	static inline const float kTurnBrakeRate = 0.35f;
	// 最高速度（1フレームあたり）
	static inline const float kLimitRunSpeed = 0.18f;
	// 空中での左右の加速度（地上より弱め）
	static inline const float kAirAcceleration = 0.015f;
	// 左右の入力があるか（着地時の減速に使う）
	bool hasMoveInput_ = false;

	// 接地状態フラグ
	bool onGround_ = true;

	// 重力加速度（落下中）
	static inline const float kGravityAcceleration = 0.08f;
	// 上昇中の重力（落下中より弱くして、ふわっと上がってストンと落ちる）
	static inline const float kGravityRise = 0.05f;
	// 最大落下速度（下方向）
	static inline const float kLimitFallSpeed = 0.5f;
	// ジャンプ初速（上方向）
	static inline const float kJumpAcceleration = 0.65f;
	static inline const float kStompBounceSpeed = 0.45f;
	// ジャンプボタンを押したまま敵を踏んだ時の跳ね返り
	static inline const float kStompBounceHighSpeed = 0.65f;
	// 上昇中にジャンプボタンを離した時に上向きの速度に掛ける割合（小ジャンプ）
	static inline const float kJumpCutRate = 0.4f;
	// 足場から落ちた後もジャンプできる猶予時間<秒>（コヨーテタイム）
	static inline const float kCoyoteTime = 0.1f;
	// 着地前に押したジャンプを覚えておく時間<秒>（先行入力）
	static inline const float kJumpBufferTime = 0.12f;

	// ジャンプボタンを押したまま上昇中か
	bool isJumping_ = false;
	float coyoteTimer_ = 0.0f;
	float jumpBufferTimer_ = 0.0f;

	//キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	static inline const float kBlank = 0.005f;

	// 着地時の速度減衰率
	static inline const float kAttenuationLanding = 0.2f;

	// 壁接触時の速度減衰率
	static inline const float kAttenuationWall = 0.2f;

	// 残りの無敵時間<秒>
	float invincibleTimer_ = 0.0f;
	// 無敵中の点滅の速さ（1秒あたりの切り替え回数）
	static inline const float kBlinkRate = 20.0f;

	bool isDead_ = false;
};
