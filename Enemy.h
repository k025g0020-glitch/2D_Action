#pragma once
#include "KamataEngine.h"
#include "CollisionMapInfo.h"
#include "SmokeParticles.h"
#include <array>
#include <cassert>

class MapChipField;

class Player;

class Enemy {
public:
	enum class Type {
		kGround,     // 通常の地上敵
		kFlying,     // 空を飛ぶ敵
		kInvincible, // 倒せない固定敵
	};

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, MapChipField* mapChipField, const KamataEngine::Vector3& position, Type type = Type::kGround, KamataEngine::Model* smokeModel = nullptr);
	void Update();
	void Draw();

	// 手足を動かすための部品モデルを設定する（設定しない時は Initialize のモデルをそのまま描く）
	// arm と leg は左右で同じモデルを使う
	void SetPartModels(KamataEngine::Model* body, KamataEngine::Model* arm, KamataEngine::Model* leg);
	// 飛ぶ敵（ドローン）の機体とプロペラのモデルを設定する。プロペラは4つとも同じモデルを使う
	void SetDroneModels(KamataEngine::Model* body, KamataEngine::Model* rotor);
	// 倒せない敵（警備ロボット）の胴体・頭・警告灯のモデルを設定する
	void SetRobotModels(KamataEngine::Model* body, KamataEngine::Model* head, KamataEngine::Model* beacon);

	const KamataEngine::Vector3& GetTranslation() const { return worldTransform_.translation_; }
	void SetTranslation(const KamataEngine::Vector3& translation) { worldTransform_.translation_ = translation; }
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	KamataEngine::Vector3 GetWorldPosition();
	AABB GetAABB();

	void OnCollision(const Player* player);
	void Defeat();
	bool IsDefeated() const;
	bool CanBeDefeated() const;


private:
	void UpdateFlying();
	void UpdateWorldTransform();
	// 進行方向を反転する
	void TurnAround();
	// 進行方向の足元に床がないか（足場の端か）
	bool IsEdgeAhead();

	// これより下に落ちたら消える
	static inline const float kFallOutY = -2.0f;

	Type type_ = Type::kGround;

	// 飛行敵の基準座標
	KamataEngine::Vector3 flyingCenterPosition_ = {};

	float flyingTimer_ = 0.0f;

	// 1秒間に進む距離
	static inline const float kFlyingMoveSpeed = 1.5f;

	// 上下の振れ幅
	static inline const float kFlyingHeight = 1.5f;

	// 1秒あたりの上下運動回数
	static inline const float kFlyingFrequency = 0.5f;

	// スタート側の折り返し位置
	static inline const float kFlyingLeftLimit = 2.0f;

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

	// 接地状態の切り替え処理
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

	enum class State {
		kAlive,
		kSquashed,
		kGone,
	};

	State state_ = State::kAlive;

	float squashTimer_ = 0.0f;
	float squashStartY_ = 0.0f;

	static inline const float kSquashDuration = 0.12f;
	static inline const float kSquashWaitTime = 0.15f;
	static inline const float kSquashedScaleY = 0.4f;

	// 倒された時に煙になって消える演出
	SmokeParticles smoke_;
	KamataEngine::Model* smokeModel_ = nullptr;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::Vector3 velocity_ = {};
	MapChipField* mapChipField_ = nullptr;

	LRDirection lrDirection_ = LRDirection::kRight;

	// 旋回制御用メンバ
	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;
	static inline const float kTimeTurn = 0.3f;

	// エネミー用の移動パラメータ（必要に応じて数値を調整してください）
	static inline const float kAcceleration = 0.05f;
	static inline const float kAttenuation = 0.1f;
	static inline const float kLimitRunSpeed = 0.10f; // プレイヤーより少し遅め

	// 接地状態フラグ
	bool onGround_ = true;

	// 重力・ジャンプパラメータ
	static inline const float kGravityAcceleration = 0.08f;
	static inline const float kLimitFallSpeed = 0.5f;

	// 当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;
	static inline const float kBlank = 0.005f;

	// 各種減衰率
	static inline const float kAttenuationLanding = 0.2f;
	static inline const float kAttenuationWall = 0.2f;

	// アニメーション
	static inline const float kWalkMotionAngleStart = 30.0f; 
	static inline const float kWalkMotionAngleEnd = 30.0f;
	static inline const float kWalkMotionTime = 60.0f;
	float walkTimer_ = 0.0f;

	// ---- 手足のアニメーション（部品モデルがある時） ----
	// 部品（Near は画面の手前側、Far は奥側）
	enum Part {
		kBody,
		kArmNear,
		kArmFar,
		kLegNear,
		kLegFar,
		kNumParts,
	};
	// 部品のワールド行列を計算する
	void UpdatePartTransforms();

	// 部品を付ける関節の位置（モデルの座標。suitman_*.obj と合わせる）
	static inline const KamataEngine::Vector3 kShoulderNear = {0.18f, 0.17f, 0.0f};
	static inline const KamataEngine::Vector3 kShoulderFar = {-0.18f, 0.17f, 0.0f};
	static inline const KamataEngine::Vector3 kHipNear = {0.07f, -0.08f, 0.0f};
	static inline const KamataEngine::Vector3 kHipFar = {-0.07f, -0.08f, 0.0f};
	// 歩く時の手足の振れ幅（ラジアン）と、1ブロック進むごとに進む振りの角度
	static inline const float kLegSwing = 0.6f;
	static inline const float kArmSwing = 0.5f;
	static inline const float kStrideRate = 3.2f;

	std::array<KamataEngine::WorldTransform, kNumParts> partTransforms_;
	KamataEngine::Model* bodyModel_ = nullptr;
	KamataEngine::Model* armModel_ = nullptr;
	KamataEngine::Model* legModel_ = nullptr;
	bool hasPartModels_ = false;
	// 手足の振り（前に振る向きが正）と、振りの周期
	float limbSwing_ = 0.0f;
	float walkPhase_ = 0.0f;

	// ---- ドローン（飛ぶ敵）：partTransforms_ の 0 番を機体、1〜4 番をプロペラに使う ----
	static inline const std::array<KamataEngine::Vector3, 4> kRotorPositions = {{
	    {0.2f, 0.10f, 0.3f},
	    {-0.2f, 0.10f, 0.3f},
	    {0.2f, 0.10f, -0.3f},
	    {-0.2f, 0.10f, -0.3f},
	}};
	// プロペラの回る速さ（ラジアン/フレーム）
	static inline const float kRotorSpinSpeed = 0.7f;
	// 進む向きに機首を少し下げる角度
	static inline const float kDroneForwardTilt = 0.15f;

	KamataEngine::Model* rotorModel_ = nullptr;
	bool hasDroneModels_ = false;
	float rotorAngle_ = 0.0f;

	// ---- 警備ロボット（倒せない敵）：partTransforms_ の 0 番を胴体、1 番を頭、2 番を警告灯に使う ----
	// 首の位置と、首から見た警告灯の付け根の位置（robot_*.obj と合わせる）
	static inline const KamataEngine::Vector3 kRobotNeck = {0.0f, 0.16f, 0.0f};
	static inline const KamataEngine::Vector3 kRobotBeacon = {0.0f, 0.27f, 0.0f};
	// 頭を左右に振って見張る角度と速さ、警告灯の回る速さ
	static inline const float kScanAngle = 0.6f;
	static inline const float kScanSpeed = 1.2f;
	static inline const float kBeaconSpinSpeed = 6.0f;

	KamataEngine::Model* headModel_ = nullptr;
	KamataEngine::Model* beaconModel_ = nullptr;
	bool hasRobotModels_ = false;
	float robotTimer_ = 0.0f;
};
