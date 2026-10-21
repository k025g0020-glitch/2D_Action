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
};
