#include "GameScene.h"
#include <cmath>

using namespace KamataEngine;

void GameScene::Initialize() {
	model_ = Model::CreateFromOBJ("player", true);
	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	modelInvincibleEnemy_ = Model::CreateFromOBJ("shieldEnemy", true);
	modelBlock_ = Model::CreateFromOBJ("block", true);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	modelDeathParticle_ = Model::CreateFromOBJ("deathParticle", true);

	camera_.Initialize();
	debugCamera_ = new DebugCamera(1280, 720);

	skydome_ = new Skydome();
	skydome_->Initialize(modelSkydome_);

	mapChipField_ = new MapChipField;
	mapChipField_->LoadMapChipCsv("Resources/blocks.csv");

	Vector3 playerInitialPosition = mapChipField_->GetMapChipPositionByIndex(2, 18);

	player_ = new Player();
	player_->Initialize(model_, &camera_, mapChipField_, playerInitialPosition);
	player_->SetMapChipField(mapChipField_);

	// 通常の地上敵
	const uint32_t enemyColumns[] = {12, 26, 40};

	for (uint32_t enemyColumn : enemyColumns) {
		Enemy* enemy = new Enemy();

		const Vector3 position = mapChipField_->GetMapChipPositionByIndex(enemyColumn, 18);

		enemy->Initialize(modelEnemy_, &camera_, mapChipField_, position, Enemy::Type::kGround);

		enemies_.push_back(enemy);
	}

	// ゴール側からスタート側へ飛ぶ敵
	{
		Enemy* flyingEnemy = new Enemy();

		// X=46付近のゴール側、高さY=9付近
		const Vector3 position = mapChipField_->GetMapChipPositionByIndex(46, 10);

		flyingEnemy->Initialize(modelEnemy_, &camera_, mapChipField_, position, Enemy::Type::kFlying);

		enemies_.push_back(flyingEnemy);
	}

	// 動かない無敵敵
	{
		Enemy* invincibleEnemy = new Enemy();

		const Vector3 position = mapChipField_->GetMapChipPositionByIndex(34, 18);

		invincibleEnemy->Initialize(modelInvincibleEnemy_, &camera_, mapChipField_, position, Enemy::Type::kInvincible);

		enemies_.push_back(invincibleEnemy);
	}

	// ステージ右端のゴールポールと旗
	goalPole_.Initialize();
	goalPole_.translation_ = {47.0f, 3.0f, 0.0f};
	goalPole_.scale_ = {0.15f, 3.0f, 0.15f};
	goalFlag_.Initialize();
	goalFlag_.translation_ = {46.45f, 5.25f, 0.0f};
	goalFlag_.scale_ = {0.7f, 0.45f, 0.15f};

	deathParticles_ = nullptr;

	cameraController_ = new CameraController();
	cameraController_->Initialize(&camera_);
	cameraController_->SetTarget(player_);
	// カメラ中心ではなく画面端がステージ境界を越えない範囲に制限する
	constexpr float kCameraDistance = 15.0f;
	constexpr float kStageLeft = -0.5f;
	constexpr float kStageRight = 49.5f;
	constexpr float kStageBottom = -0.5f;
	constexpr float kStageTop = 19.5f;
	const float cameraHalfHeight = std::tan(camera_.fovAngleY * 0.5f) * kCameraDistance;
	const float cameraHalfWidth = cameraHalfHeight * camera_.aspectRatio;
	Rect area = {
	    kStageLeft + cameraHalfWidth,
	    kStageRight - cameraHalfWidth,
	    kStageBottom + cameraHalfHeight,
	    kStageTop - cameraHalfHeight,
	};
	cameraController_->SetMovableArea(area);
	cameraController_->Reset();

	GenerateBlocks();

	// 生成した直後にブロックのワールド行列を計算＆定数バッファへ転送しておく
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			Matrix4x4 affineMatrix = calc::MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = affineMatrix;
			worldTransformBlock->TransferMatrix();
		}
	}

	// カメラの初期状態を行列に反映
	cameraController_->Update();
	goalPole_.matWorld_ = calc::MakeAffineMatrix(goalPole_.scale_, goalPole_.rotation_, goalPole_.translation_);
	goalPole_.TransferMatrix();
	goalFlag_.matWorld_ = calc::MakeAffineMatrix(goalFlag_.scale_, goalFlag_.rotation_, goalFlag_.translation_);
	goalFlag_.TransferMatrix();

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);

	phase_ = Phase::kFadeIn;
}

void GameScene::Update() {

	if (fade_) {
		fade_->Update();
	}

	ChangePhase();

	switch (phase_) {
	case Phase::kFadeIn:
		// 背景やキャラクターの更新を行う
		skydome_->Update();
		player_->Update();
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

		if (!isDebugCameraActive_) {
			cameraController_->Update();
		}
		break;

	case Phase::kPlay:
		UpdatePlayPhase();
		break;

	case Phase::kDeath:
		UpdateDeathPhase();
		break;

	case Phase::kFadeOut:
		// デス演出完了後の背景等の更新
		skydome_->Update();
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}
		if (deathParticles_) {
			deathParticles_->Update();
		}
		break;
	}
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kFadeIn:
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay:
		// 自キャラがデス状態か？
		if (player_->IsDead()) {
			// 死亡演出フェーズに切り替え
			phase_ = Phase::kDeath;

			// 自キャラの座標を取得
			const Vector3& deathParticlesPosition = player_->GetWorldPosition();

			// 自キャラの座標にデスパーティクルを発生、初期化
			deathParticles_ = new DeathParticles();
			deathParticles_->Initialize(modelDeathParticle_, &camera_, deathParticlesPosition);
		}
		break;

	case Phase::kDeath:
		// デスパーティクルが有効で演出が終了したらフェードアウト開始
		if (deathParticles_ && deathParticles_->IsFinished()) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
		}
		break;

	case Phase::kFadeOut:
		// フェードアウトが完了したらシーン終了
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

// ゲームプレイフェーズの処理
void GameScene::UpdatePlayPhase() {
	// 天球の更新
	skydome_->Update();

	// 自キャラの更新
	player_->Update();

	// 敵の更新 (複数)
	for (Enemy* enemy : enemies_) {
		enemy->Update();
	}

	// カメラコントローラの更新＆カメラの更新
	if (!isDebugCameraActive_) {
		cameraController_->Update();
	}
	debugCamera_->Update();

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	}

	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			Matrix4x4 affineMatrix = calc::MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = affineMatrix;
			worldTransformBlock->TransferMatrix();
		}
	}

	// 全ての当たり判定
	CheckAllCollisions();

	if (!player_->IsDead() && IsGoalReached()) {
		isCleared_ = true;
		phase_ = Phase::kFadeOut;
		fade_->Start(Fade::Status::FadeOut, kFadeDuration);
	}
}

// デス演出フェーズの処理
void GameScene::UpdateDeathPhase() {
	// 天球の更新
	skydome_->Update();

	// 敵の更新 (複数)
	for (Enemy* enemy : enemies_) {
		enemy->Update();
	}

	// デスパーティクルの更新
	if (deathParticles_) {
		deathParticles_->Update();
	}

	// カメラの更新
	debugCamera_->Update();

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	}

	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			Matrix4x4 affineMatrix = calc::MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = affineMatrix;
			worldTransformBlock->TransferMatrix();
		}
	}
}

void GameScene::Draw() {
	Model::PreDraw();
	skydome_->Draw(camera_);

	if (phase_ == Phase::kPlay || phase_ == Phase::kFadeIn || (phase_ == Phase::kFadeOut && isCleared_)) {
		player_->Draw();
	}

	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}

	if (deathParticles_) {
		deathParticles_->Draw();
	}

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			modelBlock_->Draw(*worldTransformBlock, camera_);
		}
	}
	//modelBlock_->Draw(goalPole_, camera_);
	//modelBlock_->Draw(goalFlag_, camera_);
	Model::PostDraw();

	if (fade_) {
		fade_->Draw();
	}
}

void GameScene::GenerateBlocks() {
	// 要素数をマップチップから取得
	uint32_t numBlockVertical = mapChipField_->GetNumBlockVirtical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	// 列数を設定（縦方向のブロック数）
	worldTransformBlocks_.resize(numBlockVertical);
	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		// 1列の要素数を設定（横方向のブロック数）
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	// ブロックの生成
	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			// マップチップデータに沿って配置する
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();

				worldTransformBlocks_[i][j] = worldTransform;
				// 座標をマップチップから取得して設定
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			} else {
				// 空きマスには nullptr を入れる
				worldTransformBlocks_[i][j] = nullptr;
			}
		}
	}
}

void GameScene::CheckAllCollisions() {
	AABB playerAABB = player_->GetAABB();

	for (Enemy* enemy : enemies_) {
		if (enemy->IsDefeated()) {
			continue;
		}

		const AABB enemyAABB = enemy->GetAABB();

		if (!calc::IsCollision(playerAABB, enemyAABB)) {
			continue;
		}

		const float playerCenterY = (playerAABB.min.y + playerAABB.max.y) * 0.5f;

		const float enemyCenterY = (enemyAABB.min.y + enemyAABB.max.y) * 0.5f;

		const bool stompedFromAbove = player_->GetVelocity().y < 0.0f && playerCenterY > enemyCenterY + 0.2f;

		if (stompedFromAbove && enemy->CanBeDefeated()) {

			enemy->Defeat();
			player_->BounceFromEnemy();

		} else {
			// 横からの接触、または無敵敵への攻撃
			player_->OnCollision(enemy);
		}
	}
}

GameScene::~GameScene() {
	delete model_;
	delete player_;
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	delete modelBlock_;
	delete skydome_;
	delete modelSkydome_;
	delete mapChipField_;
	delete cameraController_;
	if (deathParticles_) {
		delete deathParticles_;
	}

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}

	worldTransformBlocks_.clear();
	delete debugCamera_;

	if (fade_) {
		delete fade_;
	}

	delete modelEnemy_;
	delete modelInvincibleEnemy_;
	delete modelDeathParticle_;
}

bool GameScene::IsGoalReached() {
	const Vector3 playerPosition = player_->GetWorldPosition();

	return playerPosition.x >= goalPole_.translation_.x - 0.6f;
}