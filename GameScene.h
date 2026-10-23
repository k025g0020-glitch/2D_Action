#pragma once
#include "BitmapText.h"
#include "CameraController.h"
#include "CityBackground.h"
#include "Checkpoint.h"
#include "Coin.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Fade.h"
#include "GameSession.h"
#include "GoalStar.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "ParticleSystem.h"
#include "Player.h"
#include "calc.h"
#include <list>
#include <string>
#include <vector>

class GameScene {
public:
	enum class Phase {
		kFadeIn,  // フェードイン
		kPlay,    // ゲームプレイ
		kDeath,   // デス演出
		kFadeOut, // フェードアウト
	};

	// シーン終了時の結果
	enum class Result {
		kNone,    // まだ終わっていない
		kCleared, // ゴールした
		kMiss,    // ミス（敵に当たる・穴に落ちる・時間切れ）
	};

	// session : 残機・スコアなどゲーム全体で引き継ぐ情報
	void Initialize(GameSession* session);
	void Update();
	void Draw();
	void GenerateBlocks();
	~GameScene();
	void CheckAllCollisions();
	void ChangePhase();
	bool IsFinished() const { return finished_; }
	Result GetResult() const { return result_; }

private:
	// 画面に一瞬表示される得点などの文字
	struct ScorePopup {
		KamataEngine::Vector3 position;
		std::string text;
		float timer;
	};

	void UpdatePlayPhase();
	void UpdateDeathPhase();
	// 天球・敵・コインなど、どのフェーズでも動かすオブジェクトの更新
	void UpdateStageObjects();
	void UpdateDebugCamera();
	bool IsGoalReached();
	// CSVに置かれたコイン・敵・ゴールなどを生成する
	void SpawnObjectsFromMap();
	// 敵を連続で踏んだ時の得点
	void AddStompScore(const KamataEngine::Vector3& position);
	void AddPopup(const KamataEngine::Vector3& position, const std::string& text);
	void StartDeath();
	// プレイヤーの動き（ジャンプ・着地・ダッシュ・ブレーキ）に合わせてパーティクルを出す
	void EmitPlayerEffects();
	void DrawHud();
	KamataEngine::Vector2 WorldToScreen(const KamataEngine::Vector3& position) const;

	static inline const float kFadeDuration = 1.0f;
	// 復活直後の無敵時間（フェードイン中も減るので、操作できるのは約2秒）
	static inline const float kRespawnInvincibleTime = 3.0f;
	// これより下に落ちたらミス
	static inline const float kFallDeathY = -1.0f;
	// 残り時間がこれ以下になったら時間表示を赤くする
	static inline const float kHurryTime = 30.0f;
	static inline const uint32_t kCoinScore = 200;
	// 着地せずに連続で敵を踏んだ時の得点（これを超えると1UP）
	static inline const uint32_t kStompScores[] = {100, 200, 400, 500, 800, 1000, 2000, 4000, 5000, 8000};
	static inline const float kPopupDuration = 0.8f;
	static inline const float kPopupRiseSpeed = 0.03f;

	Phase phase_ = Phase::kFadeIn;

	bool finished_ = false;
	Result result_ = Result::kNone;

	GameSession* session_ = nullptr;

	// 残り時間（秒）
	float remainingTime_ = 0.0f;
	// このステージで取ったコインと倒した敵の数
	uint32_t stageCoins_ = 0;
	uint32_t defeatedEnemies_ = 0;
	// 着地せずに連続で敵を踏んだ回数
	uint32_t stompCombo_ = 0;

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	uint32_t textureHandle_ = 0;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* modelBlock_ = nullptr;
	// 地上の敵：黒いスーツの男（1つにまとめたモデルと、手足を動かすための部品）
	KamataEngine::Model* modelSuitman_ = nullptr;
	KamataEngine::Model* modelSuitmanBody_ = nullptr;
	KamataEngine::Model* modelSuitmanArm_ = nullptr;
	KamataEngine::Model* modelSuitmanLeg_ = nullptr;
	// 飛ぶ敵：監視ドローン（1つにまとめたモデルと、機体・プロペラ）
	KamataEngine::Model* modelDrone_ = nullptr;
	KamataEngine::Model* modelDroneBody_ = nullptr;
	KamataEngine::Model* modelDroneRotor_ = nullptr;
	// 倒せない敵：装甲の警備ロボット（胴体・頭・警告灯）
	KamataEngine::Model* modelRobotBody_ = nullptr;
	KamataEngine::Model* modelRobotHead_ = nullptr;
	KamataEngine::Model* modelRobotBeacon_ = nullptr;
	KamataEngine::Model* modelInvincibleEnemy_ = nullptr;
	KamataEngine::Model* modelDeathParticle_ = nullptr;
	KamataEngine::Model* modelGoalStar_ = nullptr;
	KamataEngine::Model* modelSmoke_ = nullptr;
	KamataEngine::Model* modelCoin_ = nullptr;
	KamataEngine::Model* modelCheckpoint_ = nullptr;
	// プレイヤーの手足を動かすための部品
	KamataEngine::Model* modelPlayerBody_ = nullptr;
	KamataEngine::Model* modelPlayerArm_ = nullptr;
	KamataEngine::Model* modelPlayerLeg_ = nullptr;
	KamataEngine::Model* modelPlayerScarf_ = nullptr;

	KamataEngine::Camera camera_;

	CameraController* cameraController_ = nullptr;
	// 背景の空とビル群
	CityBackground* cityBackground_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	Player* player_ = nullptr;
	std::list<Enemy*> enemies_;
	std::list<Coin*> coins_;
	Checkpoint* checkpoint_ = nullptr;
	DeathParticles* deathParticles_ = nullptr;
	GoalStar* goalStar_ = nullptr;
	Fade* fade_ = nullptr;

	// 砂ぼこり・キラキラなどの演出
	ParticleSystem particles_;
	// 一定間隔で出す演出用のフレームカウンタ
	uint32_t effectFrame_ = 0;
	// 走っている時の砂ぼこりを出す間隔（フレーム）
	static inline const uint32_t kRunDustInterval = 4;
	// ゴールの星のまわりで光る間隔（フレーム）
	static inline const uint32_t kTwinkleInterval = 10;

	BitmapText text_;
	std::list<ScorePopup> popups_;

	bool isDebugCameraActive_ = false;
	bool isCleared_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};
