#define NOMINMAX
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "Enemy.h"
#include "Player.h"
#include "MapChipField.h"
#include "calc.h"

using namespace KamataEngine;

void Enemy::Initialize(Model* model, Camera* camera, MapChipField* mapChipField, const Vector3& position, Type type, Model* smokeModel) {
	assert(model);
	assert(mapChipField);

	model_ = model;
	camera_ = camera;
	mapChipField_ = mapChipField;
	type_ = type;
	smokeModel_ = smokeModel;

	if (smokeModel_) {
		smoke_.Initialize(smokeModel_, camera_);
	}

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	lrDirection_ = LRDirection::kRight;

	// 飛ぶ敵はスタート側（左）へ飛ぶので左を向く
	if (type_ == Type::kFlying) {
		worldTransform_.rotation_.y = std::numbers::pi_v<float> * 3.0f / 2.0f;
		lrDirection_ = LRDirection::kLeft;
	}

	turnFirstRotationY_ = worldTransform_.rotation_.y;
	turnTimer_ = 0.0f;
	walkTimer_ = 0.0f;

	onGround_ = true;

	state_ = State::kAlive;
	squashTimer_ = 0.0f;

	flyingCenterPosition_ = position;
	flyingTimer_ = 0.0f;

	velocity_ = {};

	UpdateWorldTransform();
}

void Enemy::Update() {
	if (state_ == State::kGone) {
		// 消えた後も煙の演出だけ続ける
		smoke_.Update();
		return;
	}

	// 踏まれた後のつぶれアニメーション
	if (state_ == State::kSquashed) {
		squashTimer_ += 1.0f / 60.0f;

		const float ratio = std::clamp(squashTimer_ / kSquashDuration, 0.0f, 1.0f);

		worldTransform_.scale_.y = std::lerp(1.0f, kSquashedScaleY, ratio);

		// 足元を動かさず、上から押しつぶす
		worldTransform_.translation_.y = squashStartY_ - kHeight * (1.0f - worldTransform_.scale_.y) * 0.5f;

		UpdateWorldTransform();

		if (squashTimer_ >= kSquashDuration + kSquashWaitTime) {
			state_ = State::kGone;

			// 煙になって消える
			if (smokeModel_) {
				smoke_.Start({worldTransform_.translation_.x, squashStartY_, worldTransform_.translation_.z});
			}
		}

		return;
	}

	// 飛行敵はマップ衝突や重力を使用しない
	if (type_ == Type::kFlying) {
		UpdateFlying();
		return;
	}

	// 無敵敵は初期位置に固定
	if (type_ == Type::kInvincible) {
		velocity_ = {};
		// 頭を振って見張り、警告灯を回すための時間
		robotTimer_ += 1.0f / 60.0f;
		UpdateWorldTransform();
		return;
	}

	// ここから通常の地上敵
	// 足場の端で引き返して穴に落ちないようにする
	if (onGround_ && IsEdgeAhead()) {
		TurnAround();
	}

	Move();

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;

	MapCollision(collisionMapInfo);

	// 旋回アニメーション
	if (turnTimer_ > 0.0f) {
		turnTimer_ -= 1.0f / 60.0f;

		if (turnTimer_ < 0.0f) {
			turnTimer_ = 0.0f;
		}

		const float destinationRotationYTable[] = {
		    std::numbers::pi_v<float> / 2.0f,
		    std::numbers::pi_v<float> * 3.0f / 2.0f,
		};

		const float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		const float turnRatio = 1.0f - turnTimer_ / kTimeTurn;

		worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, turnRatio);
	}

	OnCeilingCollision(collisionMapInfo);
	OnWallCollision(collisionMapInfo);

	// 壁に当たったら進行方向を反転
	if (collisionMapInfo.hitWall) {
		TurnAround();
	}

	ApplyMove(collisionMapInfo);
	OnGroundToggle(collisionMapInfo);

	// 穴に落ちたら消える
	if (worldTransform_.translation_.y < kFallOutY) {
		state_ = State::kGone;
		return;
	}

	// 歩行アニメーション
	walkTimer_ += 1.0f / 60.0f;

	const float sinParam = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer_);

	if (hasPartModels_) {
		// 人型のモデル：進んだ距離に合わせて、脚と反対側の腕を交互に振る（足が滑って見えないように）
		walkPhase_ += std::abs(velocity_.x) * kStrideRate;
		limbSwing_ = std::sin(walkPhase_);
	} else {
		// 1つのモデル：体全体を前後に揺らす
		const float walkRatio = (sinParam + 1.0f) / 2.0f;

		const float degree = -kWalkMotionAngleStart + (kWalkMotionAngleStart + kWalkMotionAngleEnd) * walkRatio;

		worldTransform_.rotation_.x = degree * (std::numbers::pi_v<float> / 180.0f);
	}

	UpdateWorldTransform();
}

void Enemy::SetPartModels(Model* body, Model* arm, Model* leg) {
	assert(body && arm && leg);
	bodyModel_ = body;
	armModel_ = arm;
	legModel_ = leg;
	hasPartModels_ = true;

	for (WorldTransform& part : partTransforms_) {
		part.Initialize();
	}
	// 体全体の揺れは使わない
	worldTransform_.rotation_.x = 0.0f;
	UpdateWorldTransform();
}

void Enemy::SetDroneModels(Model* body, Model* rotor) {
	assert(body && rotor);
	bodyModel_ = body;
	rotorModel_ = rotor;
	hasDroneModels_ = true;

	for (WorldTransform& part : partTransforms_) {
		part.Initialize();
	}
	UpdateWorldTransform();
}

void Enemy::SetRobotModels(Model* body, Model* head, Model* beacon) {
	assert(body && head && beacon);
	bodyModel_ = body;
	headModel_ = head;
	beaconModel_ = beacon;
	hasRobotModels_ = true;

	for (WorldTransform& part : partTransforms_) {
		part.Initialize();
	}
	UpdateWorldTransform();
}

void Enemy::UpdatePartTransforms() {
	// 部品は本体の行列（向き・位置・踏まれた時のつぶれ）に付いていく
	const Matrix4x4& parent = worldTransform_.matWorld_;

	if (hasRobotModels_) {
		partTransforms_[0].matWorld_ = parent;
		partTransforms_[0].TransferMatrix();
		// 頭をゆっくり左右に振って見張る
		const float headYaw = std::sin(robotTimer_ * kScanSpeed) * kScanAngle;
		const Matrix4x4 headWorld = calc::Multiply(calc::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, headYaw, 0.0f}, kRobotNeck), parent);
		partTransforms_[1].matWorld_ = headWorld;
		partTransforms_[1].TransferMatrix();
		// 頭の上の警告灯を回す
		partTransforms_[2].matWorld_ = calc::Multiply(calc::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, robotTimer_ * kBeaconSpinSpeed, 0.0f}, kRobotBeacon), headWorld);
		partTransforms_[2].TransferMatrix();
		return;
	}

	if (hasDroneModels_) {
		// 機体は進む向きに機首を少し下げる（rotation.x がプラスで機首が下がる）
		const Matrix4x4 bodyWorld = calc::Multiply(calc::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {kDroneForwardTilt, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}), parent);
		partTransforms_[0].matWorld_ = bodyWorld;
		partTransforms_[0].TransferMatrix();
		// プロペラは軸のまわりに回す（隣どうしで逆回転・向きをずらして同じに見えないように）
		for (size_t i = 0; i < kRotorPositions.size(); ++i) {
			const float direction = (i % 3 == 0) ? 1.0f : -1.0f;
			const float angle = rotorAngle_ * direction + static_cast<float>(i) * 0.8f;
			WorldTransform& rotor = partTransforms_[i + 1];
			rotor.matWorld_ = calc::Multiply(calc::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {0.0f, angle, 0.0f}, kRotorPositions[i]), bodyWorld);
			rotor.TransferMatrix();
		}
		return;
	}

	if (!hasPartModels_) {
		return;
	}
	auto setPart = [&](Part part, float swing, const Vector3& pivot) {
		WorldTransform& transform = partTransforms_[part];
		// rotation.x がマイナスで前に振れる
		transform.matWorld_ = calc::Multiply(calc::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {-swing, 0.0f, 0.0f}, pivot), parent);
		transform.TransferMatrix();
	};

	setPart(kBody, 0.0f, {0.0f, 0.0f, 0.0f});
	setPart(kLegNear, limbSwing_ * kLegSwing, kHipNear);
	setPart(kLegFar, -limbSwing_ * kLegSwing, kHipFar);
	setPart(kArmNear, -limbSwing_ * kArmSwing, kShoulderNear);
	setPart(kArmFar, limbSwing_ * kArmSwing, kShoulderFar);
}

void Enemy::TurnAround() {
	if (lrDirection_ == LRDirection::kRight) {
		lrDirection_ = LRDirection::kLeft;
	} else {
		lrDirection_ = LRDirection::kRight;
	}

	turnFirstRotationY_ = worldTransform_.rotation_.y;

	turnTimer_ = kTimeTurn;
	velocity_.x = 0.0f;
}

bool Enemy::IsEdgeAhead() {
	const float direction = (lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;

	// 進行方向側の足元の少し先・少し下を調べる
	const Vector3 checkPosition = {
	    worldTransform_.translation_.x + direction * (kWidth / 2.0f + 0.05f),
	    worldTransform_.translation_.y - kHeight / 2.0f - 0.1f,
	    0.0f,
	};

	const MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(checkPosition);
	return mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y) != MapChipType::kBlock;
}

void Enemy::Move() {
	Vector3 acceleration = {};

	if (onGround_) {
		// 現在の進行方向に向かって加速する自動移動
		if (lrDirection_ == LRDirection::kRight) {
			acceleration.x += kAcceleration;
		} else if (lrDirection_ == LRDirection::kLeft) {
			acceleration.x -= kAcceleration;
		}

		velocity_.x += acceleration.x;
		velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);

	} else {
		// 空中（重力の加算）
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

void Enemy::UpdateFlying() {
	constexpr float kDeltaTime = 1.0f / 60.0f;

	// 前フレームのY座標
	const float previousY = worldTransform_.translation_.y;

	flyingTimer_ += kDeltaTime;

	// ゴール側からスタート側へ移動
	worldTransform_.translation_.x -= kFlyingMoveSpeed * kDeltaTime;

	// スタート側まで到達したらゴール側へ戻す
	if (worldTransform_.translation_.x <= kFlyingLeftLimit) {

		worldTransform_.translation_.x = flyingCenterPosition_.x;

		flyingTimer_ = 0.0f;
	}

	const float angle = 2.0f * std::numbers::pi_v<float> * kFlyingFrequency * flyingTimer_;

	// sinを使って上下に浮き沈みさせる
	worldTransform_.translation_.y = flyingCenterPosition_.y + std::sin(angle) * kFlyingHeight;

	// 上下運動に合わせて少し傾ける
	worldTransform_.rotation_.z = std::sin(angle) * 0.15f;

	// プロペラを回す
	rotorAngle_ += kRotorSpinSpeed;

	// 1フレームの移動量を速度へ設定
	velocity_.x = -kFlyingMoveSpeed * kDeltaTime;

	velocity_.y = worldTransform_.translation_.y - previousY;

	velocity_.z = 0.0f;

	UpdateWorldTransform();
}

void Enemy::UpdateWorldTransform() {
	worldTransform_.matWorld_ = calc::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();

	UpdatePartTransforms();
}

Vector3 Enemy::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // kRightBottom
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // kLeftBottom
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f}, // kRightTop
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f}, // kLeftTop
	};

	return {center.x + offsetTable[static_cast<uint32_t>(corner)].x, center.y + offsetTable[static_cast<uint32_t>(corner)].y, center.z + offsetTable[static_cast<uint32_t>(corner)].z};
}

void Enemy::MapCollision(CollisionMapInfo& info) {
	MapCollisionUp(info);
	MapCollisionDown(info);
	MapCollisionRight(info);
	MapCollisionLeft(info);
}

void Enemy::MapCollisionUp(CollisionMapInfo& info) {
	if (info.move.y <= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		Vector3 targetPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionsNew[i] = CornerPosition(targetPos, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		Vector3 targetTopPosBefore = {worldTransform_.translation_.x, worldTransform_.translation_.y + kHeight / 2.0f, 0.0f};
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(targetTopPosBefore);
		indexSet = mapChipField_->GetMapChipIndexByPosition(Vector3(worldTransform_.translation_.x, worldTransform_.translation_.y + info.move.y + kHeight / 2.0f, 0.0f));

		if (indexSetNow.y != indexSet.y) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.x, indexSet.y);
			float blueArrow = rect.bottom - worldTransform_.translation_.y;
			float greenArrow = (kHeight / 2.0f) + kBlank;
			float moveY = blueArrow - greenArrow;

			info.move.y = std::max(0.0f, moveY);
			info.ceilingCollision = true;
		}
	}
}

void Enemy::MapCollisionDown(CollisionMapInfo& info) {
	if (info.move.y >= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		Vector3 targetPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionsNew[i] = CornerPosition(targetPos, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		Vector3 targetBottomPosBefore = {worldTransform_.translation_.x, worldTransform_.translation_.y - kHeight / 2.0f, 0.0f};
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(targetBottomPosBefore);
		Vector3 targetBottomPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y - kHeight / 2.0f, 0.0f};
		indexSet = mapChipField_->GetMapChipIndexByPosition(targetBottomPos);

		if (indexSetNow.y != indexSet.y) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.x, indexSet.y);
			float moveY = rect.top + (kHeight / 2.0f) + kBlank - worldTransform_.translation_.y;
			info.move.y = std::min(0.0f, moveY);
			info.landing = true;
		}
	}
}

void Enemy::MapCollisionRight(CollisionMapInfo& info) {
	if (info.move.x <= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		Vector3 targetPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionsNew[i] = CornerPosition(targetPos, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x - 1, indexSet.y);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x - 1, indexSet.y);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		Vector3 targetRightPosBefore = {worldTransform_.translation_.x + kWidth / 2.0f, worldTransform_.translation_.y, 0.0f};
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(targetRightPosBefore);
		Vector3 targetRightPos = {worldTransform_.translation_.x + info.move.x + kWidth / 2.0f, worldTransform_.translation_.y + info.move.y, 0.0f};
		indexSet = mapChipField_->GetMapChipIndexByPosition(targetRightPos);

		if (indexSetNow.x != indexSet.x) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.x, indexSet.y);
			float moveX = rect.left - (kWidth / 2.0f) - kBlank - worldTransform_.translation_.x;
			info.move.x = std::max(0.0f, moveX);
			info.hitWall = true;
		}
	}
}

void Enemy::MapCollisionLeft(CollisionMapInfo& info) {
	if (info.move.x >= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		Vector3 targetPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionsNew[i] = CornerPosition(targetPos, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x + 1, indexSet.y);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x + 1, indexSet.y);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		Vector3 targetLeftPosBefore = {worldTransform_.translation_.x - kWidth / 2.0f, worldTransform_.translation_.y, 0.0f};
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(targetLeftPosBefore);
		Vector3 targetLeftPos = {worldTransform_.translation_.x + info.move.x - kWidth / 2.0f, worldTransform_.translation_.y + info.move.y, 0.0f};
		indexSet = mapChipField_->GetMapChipIndexByPosition(targetLeftPos);

		if (indexSetNow.x != indexSet.x) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.x, indexSet.y);
			float moveX = rect.right + (kWidth / 2.0f) + kBlank - worldTransform_.translation_.x;
			info.move.x = std::min(0.0f, moveX);
			info.hitWall = true;
		}
	}
}

void Enemy::OnGroundToggle(const CollisionMapInfo& info) {
	if (onGround_) {
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			MapChipType mapChipType;
			bool hit = false;
			float microValue = 0.01f;

			Vector3 leftBottomTarget = CornerPosition(worldTransform_.translation_, kLeftBottom);
			Vector3 rightBottomTarget = CornerPosition(worldTransform_.translation_, kRightBottom);
			leftBottomTarget.y -= microValue;
			rightBottomTarget.y -= microValue;

			MapChipField::IndexSet indexSet;
			indexSet = mapChipField_->GetMapChipIndexByPosition(leftBottomTarget);
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
			if (mapChipType == MapChipType::kBlock)
				hit = true;

			indexSet = mapChipField_->GetMapChipIndexByPosition(rightBottomTarget);
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
			if (mapChipType == MapChipType::kBlock)
				hit = true;

			if (!hit) {
				onGround_ = false;
			}
		}
	} else {
		if (info.landing) {
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding);
			velocity_.y = 0.0f;
		}
	}
}

void Enemy::ApplyMove(const CollisionMapInfo& info) {
	worldTransform_.translation_.x += info.move.x;
	worldTransform_.translation_.y += info.move.y;
	worldTransform_.translation_.z += info.move.z;
}

void Enemy::OnCeilingCollision(const CollisionMapInfo& info) {
	if (info.ceilingCollision) {
		velocity_.y = 0.0f;
	}
}

void Enemy::OnWallCollision(const CollisionMapInfo& info) {
	if (info.hitWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// --- ワールド座標の取得 ---
Vector3 Enemy::GetWorldPosition() {
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

// --- AABBの計算と取得 ---
AABB Enemy::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;

	// エネミーの当たり判定サイズ（kWidth, kHeight）に基づいて計算
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

// --- 衝突応答処理 ---
void Enemy::OnCollision(const Player* player) {
	(void)player;
}

bool Enemy::IsDefeated() const { return state_ != State::kAlive; }

bool Enemy::CanBeDefeated() const { return type_ != Type::kInvincible; }

void Enemy::Defeat() {
	if (state_ != State::kAlive) {
		return;
	}

	// 無敵敵には攻撃が効かない
	if (!CanBeDefeated()) {
		return;
	}

	state_ = State::kSquashed;
	squashTimer_ = 0.0f;
	squashStartY_ = worldTransform_.translation_.y;
	velocity_ = {};
}

void Enemy::Draw() {
	if (state_ != State::kGone) {
		if (hasRobotModels_) {
			bodyModel_->Draw(partTransforms_[0], *camera_);
			headModel_->Draw(partTransforms_[1], *camera_);
			beaconModel_->Draw(partTransforms_[2], *camera_);
		} else if (hasDroneModels_) {
			bodyModel_->Draw(partTransforms_[0], *camera_);
			for (size_t i = 0; i < kRotorPositions.size(); ++i) {
				rotorModel_->Draw(partTransforms_[i + 1], *camera_);
			}
		} else if (hasPartModels_) {
			bodyModel_->Draw(partTransforms_[kBody], *camera_);
			armModel_->Draw(partTransforms_[kArmNear], *camera_);
			armModel_->Draw(partTransforms_[kArmFar], *camera_);
			legModel_->Draw(partTransforms_[kLegNear], *camera_);
			legModel_->Draw(partTransforms_[kLegFar], *camera_);
		} else {
			model_->Draw(worldTransform_, *camera_);
		}
	}

	smoke_.Draw();
}
