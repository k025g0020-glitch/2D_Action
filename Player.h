#pragma once
#include "KamataEngine.h"
#include "CollisionMapInfo.h"
#include <array>
#include <cassert>
#include <cmath>

class MapChipField;

class Enemy;

class Player {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, MapChipField* mapChipField, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	// 手足を動かすための部品モデルを設定する（設定しない時は Initialize のモデルをそのまま描く）
	// arm と leg は左右で同じモデルを使う
	void SetPartModels(KamataEngine::Model* body, KamataEngine::Model* arm, KamataEngine::Model* leg, KamataEngine::Model* scarf);

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

	// ---- エフェクト用：このフレームに起きたこと ----
	// ジャンプした（GetJumpStage で何段目か分かる）
	bool HasJumped() const { return hasJumped_; }
	uint32_t GetJumpStage() const { return jumpStage_; }
	// 着地した（GetLandingSpeed は着地直前の落下の速さ）
	bool HasLanded() const { return hasLanded_; }
	float GetLandingSpeed() const { return landingSpeed_; }
	// 地上でダッシュしている
	bool IsDashingOnGround() const { return onGround_ && isDashing_ && std::abs(velocity_.x) > kLimitRunSpeed; }
	// 地上で進行方向と逆に入力してブレーキをかけている
	bool IsSkidding() const { return isSkidding_; }
	// 壁に張り付いてずり落ちている
	bool IsWallSliding() const { return isWallSliding_; }
	// 壁ジャンプした
	bool HasWallJumped() const { return hasWallJumped_; }
	// 触れている（最後に触れた）壁の向き（右の壁なら +1、左の壁なら -1）
	float GetWallDirection() const { return wallDirection_; }
	// 足元の座標
	KamataEngine::Vector3 GetFeetPosition() const {
		return {worldTransform_.translation_.x, worldTransform_.translation_.y - kHeight / 2.0f, worldTransform_.translation_.z};
	}

private:

	enum Corner {
		kRightBottom, // 右下
		kLeftBottom,  // 左下
		kRightTop,    // 右上
		kLeftTop,     // 左上
		kBottomCenter, // 下の辺の真ん中
		kTopCenter,    // 上の辺の真ん中
		kRightCenter,  // 右の辺の真ん中
		kLeftCenter,   // 左の辺の真ん中
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

	// 左右どちらかの壁に触れているか（direction は +1 で右、-1 で左）
	bool IsTouchingWall(float direction);
	// 指定した向きに振り向く
	void FaceDirection(LRDirection direction);

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

	// ダッシュ（SHIFTを押している間）の最高速度
	static inline const float kLimitDashSpeed = 0.28f;
	// 最高速度を超えている時に1フレームで落とす速度（ダッシュをやめた時に急に止まらないように）
	static inline const float kOverSpeedDeceleration = 0.008f;

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

	// 3段ジャンプ
	// 着地してからこの時間内にジャンプすると次の段になる<秒>
	static inline const float kMultiJumpWindow = 0.15f;
	// 次の段に進むのに必要な横方向の速さ（ダッシュ中でないと出ない速さ）
	static inline const float kMultiJumpMinSpeed = 0.2f;
	// 1段目・2段目・3段目のジャンプ初速
	static inline const float kMultiJumpSpeeds[] = {kJumpAcceleration, 0.75f, 0.88f};
	// 3段目の宙返りの回転速度（ラジアン/フレーム）
	static inline const float kFlipSpeed = 0.3f;

	// 今が何段目のジャンプか（0:地上で待機中, 1〜3）
	uint32_t jumpStage_ = 0;
	// 着地してからの、次の段に進める残り時間
	float multiJumpTimer_ = 0.0f;
	// 壁ずり・壁ジャンプ
	// 壁に張り付いている時の最大落下速度
	static inline const float kWallSlideSpeed = 0.1f;
	// 壁ジャンプの横方向・縦方向の初速
	static inline const float kWallJumpSpeedX = 0.2f;
	static inline const float kWallJumpSpeedY = 0.6f;
	// 壁ジャンプ直後に左右の入力を受け付けない時間<秒>（すぐ同じ壁に戻らないように）
	static inline const float kWallJumpLockTime = 0.12f;
	// 壁から離れた後も壁ジャンプできる猶予時間<秒>
	static inline const float kWallJumpGraceTime = 0.08f;
	// 壁に触れているかを調べる、体の横からの距離
	static inline const float kWallCheckDistance = 0.05f;

	float wallJumpLockTimer_ = 0.0f;
	float wallContactTimer_ = 0.0f;
	float wallDirection_ = 0.0f;
	bool isWallSliding_ = false;
	bool hasWallJumped_ = false;

	// このフレームに起きたこと（エフェクト用）
	bool hasJumped_ = false;
	bool hasLanded_ = false;
	float landingSpeed_ = 0.0f;
	bool isDashing_ = false;
	bool isSkidding_ = false;
	// これより速い時の切り返しでブレーキの砂ぼこりを出す
	static inline const float kSkidSpeed = 0.1f;

	// 3段目の宙返りの角度
	float flipAngle_ = 0.0f;
	bool isFlipping_ = false;

	//キャラクターの当たり判定サイズ
	static inline const float kWidth = 1.6f;
	static inline const float kHeight = 1.6f;
	// モデルの拡大率（モデルは 0.8 マスの大きさで作ってあるので、当たり判定に合わせて 2 倍にする）
	static inline const float kModelScale = 2.0f;

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

	// ---- 手足のアニメーション ----
	// 部品（Near は画面の手前側、Far は奥側）
	enum Part {
		kBody,
		kArmNear,
		kArmFar,
		kLegNear,
		kLegFar,
		kScarf,
		kNumParts,
	};

	// 状態に合わせてポーズを決め、少しずつ近づける
	void UpdateAnimation();
	// ポーズから部品のワールド行列を計算する
	void UpdatePartTransforms();

	// 部品を付ける関節の位置（モデルの座標。runner_*.obj と合わせる）
	static inline const KamataEngine::Vector3 kShoulderNear = {0.185f, 0.14f, 0.0f};
	static inline const KamataEngine::Vector3 kShoulderFar = {-0.185f, 0.14f, 0.0f};
	static inline const KamataEngine::Vector3 kHipNear = {0.08f, -0.07f, 0.0f};
	static inline const KamataEngine::Vector3 kHipFar = {-0.08f, -0.07f, 0.0f};
	static inline const KamataEngine::Vector3 kScarfRoot = {0.0f, 0.155f, -0.12f};

	// 走る時の手足の振り：1ブロック進むごとに進む角度と、振れ幅（歩き〜ダッシュ）
	static inline const float kRunCycleRate = 3.6f;
	static inline const float kRunSwingMin = 0.35f;
	static inline const float kRunSwingMax = 0.95f;
	// ダッシュ中の前傾の最大角度
	static inline const float kRunLeanMax = 0.3f;
	// 目標のポーズに近づく割合（1フレームあたり）
	static inline const float kPoseBlendRate = 0.3f;
	// 着地でつぶれる・ジャンプで伸びる量と、元に戻る割合
	static inline const float kLandSquash = 0.22f;
	static inline const float kJumpStretch = 0.15f;
	static inline const float kSquashRecoverRate = 0.2f;

	std::array<KamataEngine::WorldTransform, kNumParts> partTransforms_;
	KamataEngine::Model* bodyModel_ = nullptr;
	KamataEngine::Model* armModel_ = nullptr;
	KamataEngine::Model* legModel_ = nullptr;
	KamataEngine::Model* scarfModel_ = nullptr;
	bool hasPartModels_ = false;

	// 今のポーズ（手足は前に振る向きを正とする角度）
	float armNearAngle_ = 0.0f;
	float armFarAngle_ = 0.0f;
	float legNearAngle_ = 0.0f;
	float legFarAngle_ = 0.0f;
	float bodyLean_ = 0.0f;
	float bodyBob_ = 0.0f;
	float scarfAngle_ = -1.0f;
	KamataEngine::Vector3 squash_ = {1.0f, 1.0f, 1.0f};
	// 走りの周期と、待機・マフラー用の時間
	float runPhase_ = 0.0f;
	float animationTime_ = 0.0f;
};
