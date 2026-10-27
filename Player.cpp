#define NOMINMAX
#include <algorithm>
#include <cmath>
#include <iterator>
#include <array>
#include <numbers>

#include "Enemy.h"
#include "MapChipField.h"
#include "Player.h"
#include "calc.h"

using namespace KamataEngine;

void Player::Initialize(Model* model, Camera* camera, MapChipField* mapChipField, const Vector3& position) {
	assert(model);
	assert(mapChipField);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	// 体が1マスより大きいので、足の裏がマスの下の辺にそろうように持ち上げる
	worldTransform_.translation_.y += kHeight / 2.0f - 0.5f;
	worldTransform_.scale_ = {kModelScale, kModelScale, kModelScale};
	mapChipField_ = mapChipField;

	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	lrDirection_ = LRDirection::kRight;

	turnFirstRotationY_ = worldTransform_.rotation_.y;
	turnTimer_ = 0.0f;

	onGround_ = true;
}

void Player::Update() {
	// エフェクト用のフラグはフレームごとにリセットする
	hasJumped_ = false;
	hasLanded_ = false;
	isSkidding_ = false;
	// 無敵時間を減らす
	if (invincibleTimer_ > 0.0f) {
		invincibleTimer_ = std::max(invincibleTimer_ - 1.0f / 60.0f, 0.0f);
	}

	Move();

	// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo;
	// 移動量に速度の値をコピー
	collisionMapInfo.move = velocity_;

	// 衝突判定関数を呼び出す
	MapCollision(collisionMapInfo);

	// 旋回制御
	if (turnTimer_ > 0.0f) {
		turnTimer_ -= 1.0f / 60.0f;
		if (turnTimer_ < 0.0f) {
			turnTimer_ = 0.0f;
		}

		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		float ratio = 1.0f - (turnTimer_ / kTimeTurn);
		worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, ratio);
	}

	// 各方向の衝突・接触に応じた応答処理を行う
	OnCeilingCollision(collisionMapInfo);

	// 壁に接触している場合の処理
	OnWallCollision(collisionMapInfo);

	// 判定結果を反映して移動させる
	ApplyMove(collisionMapInfo);

	// 接地状態の切り替え処理
	OnGroundToggle(collisionMapInfo);

	// 3段目の宙返り（進行方向に向かって前転する）
	if (isFlipping_) {
		flipAngle_ += kFlipSpeed;
		if (flipAngle_ >= 2.0f * std::numbers::pi_v<float>) {
			flipAngle_ = 0.0f;
			isFlipping_ = false;
		}
	}
	const float flipDirection = (lrDirection_ == LRDirection::kRight) ? -1.0f : 1.0f;
	worldTransform_.rotation_.z = flipAngle_ * flipDirection;

	worldTransform_.matWorld_ = calc::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// 手足のアニメーション
	UpdateAnimation();
	UpdatePartTransforms();

	MapChipField::IndexSet playerIndex = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
}

void Player::Move() {
	constexpr float kDeltaTime = 1.0f / 60.0f;

	Input* input = Input::GetInstance();

	// 操作キー：A/D で左右移動、SPACE または W でジャンプ、SHIFT を押している間ダッシュ
	const bool pushRight = input->PushKey(DIK_D);
	const bool pushLeft = input->PushKey(DIK_A);
	const bool pushDash = input->PushKey(DIK_LSHIFT) || input->PushKey(DIK_RSHIFT);
	const bool pushJump = input->PushKey(DIK_SPACE) || input->PushKey(DIK_W);
	const bool triggerJump = input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_W);

	// 左右移動（空中でも少しだけ向きを変えられる）
	const float accelerationX = onGround_ ? kAcceleration : kAirAcceleration;
	hasMoveInput_ = pushRight || pushLeft;
	isDashing_ = pushDash && hasMoveInput_;
	// 地上で速く動いている時に逆方向へ入力したらブレーキ
	isSkidding_ = onGround_ && std::abs(velocity_.x) > kSkidSpeed && ((pushRight && velocity_.x < 0.0f) || (pushLeft && velocity_.x > 0.0f));
	Vector3 acceleration = {};
	if (pushRight) {
		// 左移動中の右入力
		if (onGround_ && velocity_.x < 0.0f) {
			// 速度と逆方向に入力中は急ブレーキ
			velocity_.x *= (1.0f - kTurnBrakeRate);
		}
		acceleration.x += accelerationX;

		// 向きを右に更新
		if (lrDirection_ != LRDirection::kRight) {
			lrDirection_ = LRDirection::kRight;
			// 旋回開始時の角度を記録する
			turnFirstRotationY_ = worldTransform_.rotation_.y;
			// 旋回タイマーに時間を設定する
			turnTimer_ = kTimeTurn;
		}
	} else if (pushLeft) {
		// 右移動中の左入力
		if (onGround_ && velocity_.x > 0.0f) {
			// 速度と逆方向に入力中は急ブレーキ
			velocity_.x *= (1.0f - kTurnBrakeRate);
		}
		acceleration.x -= accelerationX;

		// 向きを左に更新
		if (lrDirection_ != LRDirection::kLeft) {
			lrDirection_ = LRDirection::kLeft;
			// 旋回開始時の角度を記録する
			turnFirstRotationY_ = worldTransform_.rotation_.y;
			// 旋回タイマーに時間を設定する
			turnTimer_ = kTimeTurn;
		}
	}

	if (pushRight || pushLeft) {
		const float previousSpeed = std::abs(velocity_.x);
		velocity_.x += acceleration.x;

		// ダッシュ中は最高速度が上がる。ダッシュをやめた時は急に遅くならず、少しずつ落とす
		const float limitSpeed = pushDash ? kLimitDashSpeed : kLimitRunSpeed;
		if (std::abs(velocity_.x) > limitSpeed) {
			const float allowedSpeed = std::max(limitSpeed, previousSpeed - kOverSpeedDeceleration);
			velocity_.x = std::clamp(velocity_.x, -allowedSpeed, allowedSpeed);
		}
	} else if (onGround_) {
		velocity_.x *= (1.0f - kAttenuation);
	}

	// 着地してから時間が経ったら3段ジャンプの段数をリセット
	if (onGround_) {
		multiJumpTimer_ = std::max(multiJumpTimer_ - kDeltaTime, 0.0f);
		if (multiJumpTimer_ <= 0.0f) {
			jumpStage_ = 0;
		}
	}

	if (onGround_) {
		// 地上にいる間はコヨーテタイムを満タンにしておく
		coyoteTimer_ = kCoyoteTime;
		isJumping_ = false;
	} else {
		coyoteTimer_ = std::max(coyoteTimer_ - kDeltaTime, 0.0f);

		// 空中（落下速度の加算）。上昇中は重力を弱くする
		velocity_.y -= (velocity_.y > 0.0f) ? kGravityRise : kGravityAcceleration;
		// 落下速度制限（下方向への速度を一定値に丸める）
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);

		// 上昇中にジャンプボタンを離したら上昇を弱めて小ジャンプにする
		if (isJumping_ && !pushJump && velocity_.y > 0.0f) {
			velocity_.y *= kJumpCutRate;
			isJumping_ = false;
		}
		if (velocity_.y <= 0.0f) {
			isJumping_ = false;
		}
	}

	// ジャンプの先行入力
	if (triggerJump) {
		jumpBufferTimer_ = kJumpBufferTime;
	} else {
		jumpBufferTimer_ = std::max(jumpBufferTimer_ - kDeltaTime, 0.0f);
	}

	// 地上か、足場から落ちた直後ならジャンプできる
	if (jumpBufferTimer_ > 0.0f && coyoteTimer_ > 0.0f) {
		// ダッシュの勢いのまま着地直後にジャンプすると、2段目・3段目と高く跳べる
		const bool canChain = pushDash && multiJumpTimer_ > 0.0f && std::abs(velocity_.x) >= kMultiJumpMinSpeed;
		if (canChain && jumpStage_ < std::size(kMultiJumpSpeeds)) {
			++jumpStage_;
		} else {
			jumpStage_ = 1;
		}
		multiJumpTimer_ = 0.0f;

		// 3段目は宙返りする
		if (jumpStage_ == std::size(kMultiJumpSpeeds)) {
			isFlipping_ = true;
			flipAngle_ = 0.0f;
		}

		velocity_.y = kMultiJumpSpeeds[jumpStage_ - 1];
		hasJumped_ = true;
		isJumping_ = true;
		jumpBufferTimer_ = 0.0f;
		coyoteTimer_ = 0.0f;
	}
}

void Player::FaceDirection(LRDirection direction) {
	if (lrDirection_ == direction) {
		return;
	}
	lrDirection_ = direction;
	// 旋回開始時の角度を記録する
	turnFirstRotationY_ = worldTransform_.rotation_.y;
	// 旋回タイマーに時間を設定する
	turnTimer_ = kTimeTurn;
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // kRightBottom
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // kLeftBottom
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f}, // kRightTop
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f}, // kLeftTop
	    {0.0f, -kHeight / 2.0f, 0.0f},           // kBottomCenter
	    {0.0f, +kHeight / 2.0f, 0.0f},           // kTopCenter
	    {+kWidth / 2.0f, 0.0f, 0.0f},            // kRightCenter
	    {-kWidth / 2.0f, 0.0f, 0.0f},            // kLeftCenter
	};

	Vector3 result = {center.x + offsetTable[static_cast<uint32_t>(corner)].x, center.y + offsetTable[static_cast<uint32_t>(corner)].y, center.z + offsetTable[static_cast<uint32_t>(corner)].z};

	return result;
}

// マップ衝突判定の主処理
void Player::MapCollision(CollisionMapInfo& info) {
	MapCollisionUp(info);
	MapCollisionDown(info);
	MapCollisionRight(info);
	MapCollisionLeft(info);
}

// 各方向の衝突判定
void Player::MapCollisionUp(CollisionMapInfo& info) {
	if (info.move.y <= 0.0f) {
		return;
	}

	// 移動後の4つの角の座標をまとめて計算
	std::array<Vector3, kNumCorner> positionsNew;

	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		Vector3 targetPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionsNew[i] = CornerPosition(targetPos, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	// 真上の当たり判定を行う
	bool hit = false;

	// 左上点の判定
	MapChipField::IndexSet indexSet;
	// 辺の上の3点（両端の角と真ん中）を調べる。体が1マスより大きいので、角だけだと間のブロックを見落とす
	for (Corner corner : {kLeftTop, kTopCenter, kRightTop}) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[corner]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y + 1);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			hit = true;
		}
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

// 各方向の衝突判定（下方向）
void Player::MapCollisionDown(CollisionMapInfo& info) {
	if (info.move.y >= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		Vector3 targetPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionsNew[i] = CornerPosition(targetPos, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	// 左下点の判定
	MapChipField::IndexSet indexSet;
	// 辺の上の3点（両端の角と真ん中）を調べる。体が1マスより大きいので、角だけだと間のブロックを見落とす
	for (Corner corner : {kLeftBottom, kBottomCenter, kRightBottom}) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[corner]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y - 1);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			hit = true;
		}
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

// 右方向の衝突判定
void Player::MapCollisionRight(CollisionMapInfo& info) {
	if (info.move.x <= 0.0f) {
		return;
	}

	// 移動後の4つの角の座標をまとめて計算
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		Vector3 targetPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionsNew[i] = CornerPosition(targetPos, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	// 右上点の判定
	MapChipField::IndexSet indexSet;
	// 辺の上の3点（両端の角と真ん中）を調べる。体が1マスより大きいので、角だけだと間のブロックを見落とす
	for (Corner corner : {kRightTop, kRightCenter, kRightBottom}) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[corner]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x - 1, indexSet.y);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			hit = true;
		}
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

// 左方向の衝突判定
void Player::MapCollisionLeft(CollisionMapInfo& info) {
	if (info.move.x >= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		Vector3 targetPos = {worldTransform_.translation_.x + info.move.x, worldTransform_.translation_.y + info.move.y, worldTransform_.translation_.z + info.move.z};
		positionsNew[i] = CornerPosition(targetPos, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	// 左上点の判定
	MapChipField::IndexSet indexSet;
	// 辺の上の3点（両端の角と真ん中）を調べる。体が1マスより大きいので、角だけだと間のブロックを見落とす
	for (Corner corner : {kLeftTop, kLeftCenter, kLeftBottom}) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[corner]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.x + 1, indexSet.y);
		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			hit = true;
		}
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

// 接地状態の切り替え処理
void Player::OnGroundToggle(const CollisionMapInfo& info) {
	// 自キャラが接地状態？
	if (onGround_) {
		// ジャンプ開始
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			// 落下判定（左右移動や床がなくなるなど、ジャンプせずに落下を始める場合の処理）
			MapChipType mapChipType;
			bool hit = false;

			// 足元より「微小な数値」だけ下方向の座標を、左下・真ん中・右下の3か所サンプリングする
			float microValue = 0.01f;
			for (Corner corner : {kLeftBottom, kBottomCenter, kRightBottom}) {
				Vector3 target = CornerPosition(worldTransform_.translation_, corner);
				target.y -= microValue;
				MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(target);
				mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.x, indexSet.y);
				if (mapChipType == MapChipType::kBlock) {
					hit = true;
				}
			}

			// 落下開始（床が消失した）
			if (!hit) {
				// 空中状態に切り替え
				onGround_ = false;
			}
		}
	} else {
		// 空中にいる時（落下／ジャンプ）の処理
		// 着地フラグ
		if (info.landing) {
			// 接地状態に切り替える（落下を止める）
			onGround_ = true;
			hasLanded_ = true;
			landingSpeed_ = -velocity_.y;
			// 3段ジャンプの受付開始。3段目の後は1段目に戻す
			multiJumpTimer_ = (jumpStage_ < std::size(kMultiJumpSpeeds)) ? kMultiJumpWindow : 0.0f;
			// 宙返りを終わらせて姿勢を戻す
			isFlipping_ = false;
			flipAngle_ = 0.0f;
			// 着地時にX速度を減衰（左右を押し続けている時は勢いを保つ）
			if (!hasMoveInput_) {
				velocity_.x *= (1.0f - kAttenuationLanding);
			}
			// Y速度をゼロにする
			velocity_.y = 0.0f;
		}
	}
}

// 判定結果を反映して移動させる
void Player::ApplyMove(const CollisionMapInfo& info) {
	// 移動
	worldTransform_.translation_.x += info.move.x;
	worldTransform_.translation_.y += info.move.y;
	worldTransform_.translation_.z += info.move.z;
}

// 天井に接触している場合の処理
void Player::OnCeilingCollision(const CollisionMapInfo& info) {
	if (info.ceilingCollision) {
		DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");
		velocity_.y = 0.0f;
	}
}

// 壁に接触している場合の処理
void Player::OnWallCollision(const CollisionMapInfo& info) {
	// 壁接触による減衰
	if (info.hitWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// --- ワールド座標の取得 ---
Vector3 Player::GetWorldPosition() {
	Vector3 worldPos;
	// ワールド行列の平行移動成分（4行目: m[3][0], m[3][1], m[3][2]）を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

// --- AABBの計算と取得 ---
AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;

	// キャラクターの幅（kWidth）、高さ（kHeight）を半径分（/ 2.0f）としてボックスの最小・最大値を計算
	// 奥方向（Z）も同様の幅で判定を設定
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

// --- 衝突応答処理 ---
void Player::OnCollision(const Enemy* enemy) {
	(void)enemy;

	// 無敵中は敵に触れてもミスにならない
	if (IsInvincible()) {
		return;
	}

	// デスフラグを立てる
	isDead_ = true;
}

void Player::BounceFromEnemy() {
	// ジャンプボタンを押していると高く跳ねる
	Input* input = Input::GetInstance();
	const bool pushJump = input->PushKey(DIK_SPACE) || input->PushKey(DIK_W);

	velocity_.y = pushJump ? kStompBounceHighSpeed : kStompBounceSpeed;
	isJumping_ = pushJump;
	// 踏みつけの後は3段ジャンプを1段目からやり直す
	jumpStage_ = 0;
	onGround_ = false;
}

void Player::Draw() {
	// 無敵中は点滅させる
	if (IsInvincible() && static_cast<int>(invincibleTimer_ * kBlinkRate) % 2 == 0) {
		return;
	}

	if (!hasPartModels_) {
		model_->Draw(worldTransform_, *camera_);
		return;
	}

	bodyModel_->Draw(partTransforms_[kBody], *camera_);
	armModel_->Draw(partTransforms_[kArmNear], *camera_);
	armModel_->Draw(partTransforms_[kArmFar], *camera_);
	legModel_->Draw(partTransforms_[kLegNear], *camera_);
	legModel_->Draw(partTransforms_[kLegFar], *camera_);
	scarfModel_->Draw(partTransforms_[kScarf], *camera_);
}

void Player::SetPartModels(Model* body, Model* arm, Model* leg, Model* scarf) {
	assert(body && arm && leg && scarf);
	bodyModel_ = body;
	armModel_ = arm;
	legModel_ = leg;
	scarfModel_ = scarf;
	hasPartModels_ = true;

	for (WorldTransform& part : partTransforms_) {
		part.Initialize();
	}
	UpdatePartTransforms();
}

void Player::UpdateAnimation() {
	constexpr float kDeltaTime = 1.0f / 60.0f;
	animationTime_ += kDeltaTime;

	const float speed = std::abs(velocity_.x);
	const float speedRatio = std::clamp(speed / kLimitDashSpeed, 0.0f, 1.0f);
	// マフラーが細かくなびく
	const float flutter = std::sin(animationTime_ * 18.0f) * 0.12f;

	// 目標のポーズ（手足は前に振る向きが正）
	float armNear = 0.0f, armFar = 0.0f, legNear = 0.0f, legFar = 0.0f;
	float lean = 0.0f, bob = 0.0f, scarf = -1.1f;
	float blend = kPoseBlendRate;

	if (isFlipping_) {
		// 3段目の宙返り：体を丸める
		armNear = armFar = 1.2f;
		legNear = legFar = 1.6f;
		scarf = 0.3f + flutter;
	} else if (onGround_) {
		if (isSkidding_) {
			// ブレーキ：のけぞって前の足で踏ん張る
			lean = -0.25f;
			legNear = 0.7f;
			legFar = -0.2f;
			armNear = -0.8f;
			armFar = 0.9f;
			scarf = 0.2f + flutter;
		} else if (speed > 0.01f) {
			// 走り：脚と、反対側の腕を交互に振る。速いほど大きく振り、前に傾く
			runPhase_ += speed * kRunCycleRate;
			const float swing = std::sin(runPhase_) * std::lerp(kRunSwingMin, kRunSwingMax, speedRatio);
			legNear = swing;
			legFar = -swing;
			armNear = -swing * 0.9f;
			armFar = swing * 0.9f;
			lean = kRunLeanMax * speedRatio;
			bob = std::abs(std::cos(runPhase_)) * 0.05f * speedRatio;
			scarf = -1.1f + 1.3f * speedRatio + flutter * speedRatio;
			// 走りは速く動くので、ポーズにすぐ合わせる
			blend = 0.6f;
		} else {
			// 待機：ゆっくり呼吸する
			bob = std::sin(animationTime_ * 3.0f) * 0.008f;
			armNear = 0.08f;
			armFar = -0.08f;
			scarf = -1.15f + std::sin(animationTime_ * 2.0f) * 0.05f;
		}
	} else if (velocity_.y > 0.0f) {
		// 上昇中：片手を突き上げ、片脚を前に出す
		armNear = 2.6f;
		armFar = -0.5f;
		legNear = 0.9f;
		legFar = -0.5f;
		lean = 0.05f;
		scarf = -1.4f + flutter;
	} else {
		// 落下中：両手を広げてバランスを取る
		armNear = 1.4f;
		armFar = 1.0f;
		legNear = 0.35f;
		legFar = -0.15f;
		scarf = 0.5f + flutter;
	}

	armNearAngle_ = std::lerp(armNearAngle_, armNear, blend);
	armFarAngle_ = std::lerp(armFarAngle_, armFar, blend);
	legNearAngle_ = std::lerp(legNearAngle_, legNear, blend);
	legFarAngle_ = std::lerp(legFarAngle_, legFar, blend);
	bodyLean_ = std::lerp(bodyLean_, lean, kPoseBlendRate);
	bodyBob_ = std::lerp(bodyBob_, bob, 0.5f);
	scarfAngle_ = std::lerp(scarfAngle_, scarf, kPoseBlendRate);

	// 着地でつぶれ、ジャンプで縦に伸びる
	if (hasLanded_) {
		const float amount = kLandSquash * std::clamp(landingSpeed_ / kLimitFallSpeed, 0.3f, 1.0f);
		squash_ = {1.0f + amount * 0.6f, 1.0f - amount, 1.0f + amount * 0.6f};
	}
	if (hasJumped_) {
		squash_ = {1.0f - kJumpStretch * 0.6f, 1.0f + kJumpStretch, 1.0f - kJumpStretch * 0.6f};
	}
	squash_.x = std::lerp(squash_.x, 1.0f, kSquashRecoverRate);
	squash_.y = std::lerp(squash_.y, 1.0f, kSquashRecoverRate);
	squash_.z = std::lerp(squash_.z, 1.0f, kSquashRecoverRate);
}

void Player::UpdatePartTransforms() {
	if (!hasPartModels_) {
		return;
	}

	// つぶれ・伸びは足の裏を中心にする（地面から浮いたり沈んだりしないように）
	// この行列は拡大前のモデルの座標で計算するので、足の裏の高さも拡大率で割る
	const float footY = -kHeight / 2.0f / kModelScale;
	const Matrix4x4 squashMatrix = calc::MakeAffineMatrix(squash_, {0.0f, 0.0f, 0.0f}, {0.0f, footY * (1.0f - squash_.y), 0.0f});
	// 体の傾きと上下の揺れ
	const Matrix4x4 bodyLocal = calc::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, {bodyLean_, 0.0f, 0.0f}, {0.0f, bodyBob_, 0.0f});
	const Matrix4x4 bodyWorld = calc::Multiply(bodyLocal, calc::Multiply(squashMatrix, worldTransform_.matWorld_));

	auto setPart = [&](Part part, const Vector3& rotation, const Vector3& pivot, const Matrix4x4& parent) {
		WorldTransform& transform = partTransforms_[part];
		transform.matWorld_ = calc::Multiply(calc::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, rotation, pivot), parent);
		transform.TransferMatrix();
	};

	// 手足は rotation.x がマイナスで前に振れる
	setPart(kBody, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, bodyWorld);
	setPart(kArmNear, {-armNearAngle_, 0.0f, 0.0f}, kShoulderNear, bodyWorld);
	setPart(kArmFar, {-armFarAngle_, 0.0f, 0.0f}, kShoulderFar, bodyWorld);
	setPart(kLegNear, {-legNearAngle_, 0.0f, 0.0f}, kHipNear, bodyWorld);
	setPart(kLegFar, {-legFarAngle_, 0.0f, 0.0f}, kHipFar, bodyWorld);
	// マフラーの先は rotation.x がプラスで持ち上がる
	setPart(kScarf, {scarfAngle_, 0.0f, 0.0f}, kScarfRoot, bodyWorld);
}
