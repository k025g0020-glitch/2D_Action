#pragma once
#define NOMINMAX

#include "KamataEngine.h"
#include "Player.h"
#include "calc.h"
#include <algorithm>

class Player;

struct Rect {
	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;
};

class CameraController {
public:
	void Initialize(KamataEngine::Camera* camera);
	void Update();
	void Reset();
	void SetTarget(Player* target) { target_ = target; }
	void SetMovableArea(const Rect& area) { movableArea_ = area; }

	// 画面に映る範囲の半分の大きさ（ワールド座標）
	float GetViewHalfHeight() const { return kViewHalfHeight; }
	float GetViewHalfWidth() const { return kViewHalfHeight * camera_->aspectRatio; }

private:
	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;

	KamataEngine::Vector3 targetOffset_ = {0.0f, 0.0f, -15.0f};

	Rect movableArea_ = {50.0f, 100.0f, 0.0f, 100.0f};

	KamataEngine::Vector3 targetPosition_;
	float lookAheadOffsetX_ = 0.0f;
	float facingDirection_ = 1.0f;

	static inline const float kInterpolationRate = 0.1f;
	static inline const float kLookAheadInterpolationRate = 0.08f;
	static inline const float kLookAheadDistance = 3.0f;
	static inline const float kVelocityBias = 6.0f;
	static inline const float kDirectionThreshold = 0.01f;
	static inline const float kFixedCameraY = 6.0f;
	static inline const Rect kMargin = {-6.0f, 6.0f, -2.0f, 2.0f};

	// 平行投影で縦に映す範囲の半分（13ブロック分を映す）
	static inline const float kViewHalfHeight = 6.5f;
	// 平行投影の描画範囲（カメラからの距離）
	static inline const float kNearClip = 0.1f;
	static inline const float kFarClip = 1000.0f;

	// 透視投影を平行投影に置き換えて、奥行きで大きさが変わらない2Dの見た目にする
	void ApplyOrthographicProjection();
};
