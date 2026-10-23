#define NOMINMAX
#include "GameScene.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iterator>

using namespace KamataEngine;

void GameScene::Initialize(GameSession* session) {
	assert(session);
	session_ = session;

	// プレイヤー：フードとバイザーの、街を駆けるランナー
	model_ = Model::CreateFromOBJ("runner", true);
	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	modelSuitman_ = Model::CreateFromOBJ("suitman", true);
	modelSuitmanBody_ = Model::CreateFromOBJ("suitman_body", true);
	modelSuitmanArm_ = Model::CreateFromOBJ("suitman_arm", true);
	modelSuitmanLeg_ = Model::CreateFromOBJ("suitman_leg", true);
	modelInvincibleEnemy_ = Model::CreateFromOBJ("shieldEnemy", true);
	// 足場：街の屋上らしいコンクリートと鉄骨のブロック
	modelBlock_ = Model::CreateFromOBJ("cityblock", true);
	modelDeathParticle_ = Model::CreateFromOBJ("deathParticle", true);
	modelGoalStar_ = Model::CreateFromOBJ("star", true);
	modelSmoke_ = Model::CreateFromOBJ("smoke", true);
	modelCoin_ = Model::CreateFromOBJ("coin", true);
	modelCheckpoint_ = Model::CreateFromOBJ("checkpoint", true);
	modelPlayerBody_ = Model::CreateFromOBJ("runner_body", true);
	modelPlayerArm_ = Model::CreateFromOBJ("runner_arm", true);
	modelPlayerLeg_ = Model::CreateFromOBJ("runner_leg", true);
	modelPlayerScarf_ = Model::CreateFromOBJ("runner_scarf", true);

	camera_.Initialize();
	particles_.Initialize(modelSmoke_, modelGoalStar_, &camera_);
	debugCamera_ = new DebugCamera(1280, 720);

	mapChipField_ = new MapChipField;
	mapChipField_->LoadMapChipCsv(session_->GetStage().csvPath);

	// 背景の街並み（ステージごとに違う並びにする）
	cityBackground_ = new CityBackground();
	cityBackground_->Initialize(&camera_, static_cast<float>(mapChipField_->GetStageWidth()), session_->stageIndex + 1);

	// プレイヤー・敵・コイン・ゴールなどをマップから配置
	SpawnObjectsFromMap();

	deathParticles_ = nullptr;

	cameraController_ = new CameraController();
	cameraController_->Initialize(&camera_);
	cameraController_->SetTarget(player_);
	// カメラ中心ではなく画面端がステージ境界を越えない範囲に制限する
	constexpr float kStageLeft = -0.5f;
	const float kStageRight = static_cast<float>(mapChipField_->GetStageWidth()) - 0.5f;
	constexpr float kStageBottom = -0.5f;
	constexpr float kStageTop = 19.5f;
	const float cameraHalfHeight = cameraController_->GetViewHalfHeight();
	const float cameraHalfWidth = cameraController_->GetViewHalfWidth();
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
	// （ブロックは動かないので以降は更新しない）
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
	UpdateStageObjects();

	remainingTime_ = static_cast<float>(session_->GetStage().timeLimit);
	stageCoins_ = 0;
	defeatedEnemies_ = 0;
	stompCombo_ = 0;

	text_.Initialize();

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);

	phase_ = Phase::kFadeIn;
	result_ = Result::kNone;
}

void GameScene::SpawnObjectsFromMap() {
	Vector3 playerInitialPosition = mapChipField_->GetMapChipPositionByIndex(2, 18);

	const uint32_t numBlockVertical = mapChipField_->GetNumBlockVirtical();
	const uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	for (uint32_t y = 0; y < numBlockVertical; ++y) {
		for (uint32_t x = 0; x < numBlockHorizontal; ++x) {
			const Vector3 position = mapChipField_->GetMapChipPositionByIndex(x, y);

			switch (mapChipField_->GetMapChipTypeByIndex(x, y)) {
			case MapChipType::kPlayerStart:
				playerInitialPosition = position;
				break;

			case MapChipType::kCoin: {
				Coin* coin = new Coin();
				coin->Initialize(modelCoin_, &camera_, position);
				coins_.push_back(coin);
				break;
			}

			case MapChipType::kGroundEnemy:
			case MapChipType::kFlyingEnemy:
			case MapChipType::kInvincibleEnemy: {
				const MapChipType type = mapChipField_->GetMapChipTypeByIndex(x, y);
				Enemy* enemy = new Enemy();
				if (type == MapChipType::kGroundEnemy) {
					// 通常の地上敵：黒いスーツの男
					enemy->Initialize(modelSuitman_, &camera_, mapChipField_, position, Enemy::Type::kGround, modelSmoke_);
					enemy->SetPartModels(modelSuitmanBody_, modelSuitmanArm_, modelSuitmanLeg_);
				} else if (type == MapChipType::kFlyingEnemy) {
					// ゴール側からスタート側へ飛ぶ敵
					enemy->Initialize(modelEnemy_, &camera_, mapChipField_, position, Enemy::Type::kFlying, modelSmoke_);
				} else {
					// 動かない無敵敵
					enemy->Initialize(modelInvincibleEnemy_, &camera_, mapChipField_, position, Enemy::Type::kInvincible);
				}
				enemies_.push_back(enemy);
				break;
			}

			case MapChipType::kGoal: {
				// ゴールの星（獲得するとゴール）は地面から少し浮かせる
				Vector3 goalPosition = position;
				goalPosition.y += 0.3f;
				goalStar_ = new GoalStar();
				goalStar_->Initialize(modelGoalStar_, &camera_, goalPosition);
				break;
			}

			case MapChipType::kCheckpoint:
				checkpoint_ = new Checkpoint();
				checkpoint_->Initialize(modelCheckpoint_, &camera_, position, session_->checkpointReached);
				break;

			default:
				break;
			}
		}
	}

	// ゴールがないマップでも動くように、右端に星を置く
	if (!goalStar_) {
		Vector3 goalPosition = mapChipField_->GetMapChipPositionByIndex(mapChipField_->GetStageWidth() - 3, 18);
		goalPosition.y += 0.3f;
		goalStar_ = new GoalStar();
		goalStar_->Initialize(modelGoalStar_, &camera_, goalPosition);
	}

	// 中間ポイントを通過済みならそこから再開する
	if (checkpoint_ && checkpoint_->IsActivated()) {
		playerInitialPosition = checkpoint_->GetPosition();
	}

	player_ = new Player();
	player_->Initialize(model_, &camera_, mapChipField_, playerInitialPosition);
	player_->SetMapChipField(mapChipField_);
	player_->SetPartModels(modelPlayerBody_, modelPlayerArm_, modelPlayerLeg_, modelPlayerScarf_);

	// ミスした後の復活では、近くの敵にすぐ当たらないように少しの間無敵にする
	if (session_->isRespawn) {
		player_->StartInvincible(kRespawnInvincibleTime);
	}
}

void GameScene::Update() {

	if (fade_) {
		fade_->Update();
	}

	ChangePhase();

	switch (phase_) {
	case Phase::kFadeIn:
		// 背景やキャラクターの更新を行う
		player_->Update();
		UpdateStageObjects();

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
		// デス演出完了後・ゴール後の背景等の更新（星の獲得演出も続ける）
		UpdateStageObjects();
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
			StartDeath();
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

void GameScene::StartDeath() {
	// 死亡演出フェーズに切り替え
	phase_ = Phase::kDeath;
	result_ = Result::kMiss;

	// 自キャラの座標にデスパーティクルを発生、初期化
	// （穴に落ちた時も画面内に見えるように高さを補正する）
	Vector3 deathParticlesPosition = player_->GetWorldPosition();
	deathParticlesPosition.y = std::max(deathParticlesPosition.y, 0.5f);

	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(modelDeathParticle_, &camera_, deathParticlesPosition);
}

void GameScene::EmitPlayerEffects() {
	const Vector3 feet = player_->GetFeetPosition();
	const float velocityX = player_->GetVelocity().x;

	if (player_->HasJumped()) {
		particles_.EmitJumpDust(feet, player_->GetJumpStage());
		// 3段目は星の輪
		if (player_->GetJumpStage() == 3) {
			particles_.EmitStarRing(player_->GetWorldPosition());
		}
	}

	if (player_->HasLanded()) {
		particles_.EmitLandingDust(feet, player_->GetLandingSpeed());
	}

	// 壁を蹴った所と、壁をずり落ちている所に砂ぼこり
	const Vector3 center = player_->GetWorldPosition();
	const float wallDirection = player_->GetWallDirection();
	const Vector3 wallPoint = {center.x + wallDirection * 0.45f, center.y, center.z};
	if (player_->HasWallJumped()) {
		particles_.EmitWallJumpDust(wallPoint, wallDirection);
	} else if (player_->IsWallSliding() && effectFrame_ % kRunDustInterval == 0) {
		particles_.EmitRunDust({wallPoint.x, center.y - 0.3f, center.z}, -wallDirection);
	}

	// ブレーキ中は進行方向の前に（ブレーキは一瞬なので毎フレーム）、ダッシュ中は後ろに砂ぼこりを出す
	if (player_->IsSkidding()) {
		particles_.EmitRunDust(feet, velocityX > 0.0f ? 1.0f : -1.0f);
	} else if (player_->IsDashingOnGround() && effectFrame_ % kRunDustInterval == 0) {
		particles_.EmitRunDust(feet, velocityX > 0.0f ? -1.0f : 1.0f);
	}
}

void GameScene::UpdateStageObjects() {
	// 背景の更新（カメラに合わせて動かす）
	cityBackground_->Update();

	// 敵の更新 (複数)
	for (Enemy* enemy : enemies_) {
		enemy->Update();
	}

	for (Coin* coin : coins_) {
		coin->Update();
	}

	if (checkpoint_) {
		checkpoint_->Update();
	}

	// ゴールの星の更新
	goalStar_->Update();

	// 星のまわりのキラキラ（取った後はたくさん出す）
	++effectFrame_;
	{
		const AABB starAABB = goalStar_->GetAABB();
		const Vector3 starCenter = {(starAABB.min.x + starAABB.max.x) * 0.5f, (starAABB.min.y + starAABB.max.y) * 0.5f, 0.0f};
		if (goalStar_->IsCollected()) {
			if (effectFrame_ % 2 == 0) {
				particles_.EmitTwinkle(starCenter, 0.6f);
				particles_.EmitTwinkle(starCenter, 0.6f);
			}
		} else if (effectFrame_ % kTwinkleInterval == 0) {
			particles_.EmitTwinkle(starCenter, 0.5f);
		}
	}

	particles_.Update();

	// 得点表示を上に流しながら消す
	for (ScorePopup& popup : popups_) {
		popup.position.y += kPopupRiseSpeed;
		popup.timer -= 1.0f / 60.0f;
	}
	popups_.remove_if([](const ScorePopup& popup) { return popup.timer <= 0.0f; });
}

void GameScene::UpdateDebugCamera() {
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
}

// ゲームプレイフェーズの処理
void GameScene::UpdatePlayPhase() {
	// 自キャラの更新
	player_->Update();
	EmitPlayerEffects();

	UpdateStageObjects();

	// カメラコントローラの更新＆カメラの更新
	if (!isDebugCameraActive_) {
		cameraController_->Update();
	}
	UpdateDebugCamera();

	// 着地したら連続踏みの回数をリセット
	if (player_->IsOnGround()) {
		stompCombo_ = 0;
	}

	// 全ての当たり判定
	CheckAllCollisions();

	// 中間ポイントの通過
	if (checkpoint_ && !checkpoint_->IsActivated() && checkpoint_->IsPassedBy(player_->GetWorldPosition())) {
		checkpoint_->Activate();
		session_->checkpointReached = true;
		// 旗の先から紙吹雪
		particles_.EmitConfetti(calc::Add(checkpoint_->GetPosition(), Vector3(0.4f, 1.3f, 0.0f)));
		AddPopup(calc::Add(checkpoint_->GetPosition(), Vector3(0.0f, 2.0f, 0.0f)), "CHECKPOINT");
	}

	// 穴に落ちたらミス
	if (player_->GetWorldPosition().y < kFallDeathY) {
		player_->Die();
	}

	// 残り時間を減らし、0になったらミス
	remainingTime_ = std::max(remainingTime_ - 1.0f / 60.0f, 0.0f);
	if (remainingTime_ <= 0.0f) {
		player_->Die();
	}

	// 星を獲得したらゴール
	if (!player_->IsDead() && IsGoalReached()) {
		goalStar_->Collect();
		{
			const AABB starAABB = goalStar_->GetAABB();
			const Vector3 starCenter = {(starAABB.min.x + starAABB.max.x) * 0.5f, (starAABB.min.y + starAABB.max.y) * 0.5f, 0.0f};
			particles_.EmitSparkles(starCenter, {1.0f, 0.85f, 0.3f, 1.0f}, 24, 0.2f);
			particles_.EmitStarRing(starCenter);
		}
		isCleared_ = true;
		result_ = Result::kCleared;

		// リザルト画面用にこのステージの結果を記録する
		session_->lastStageResult.coins = stageCoins_;
		session_->lastStageResult.defeatedEnemies = defeatedEnemies_;
		session_->lastStageResult.remainingTime = static_cast<uint32_t>(std::ceil(remainingTime_));

		phase_ = Phase::kFadeOut;
		fade_->Start(Fade::Status::FadeOut, kFadeDuration);
	}
}

// デス演出フェーズの処理
void GameScene::UpdateDeathPhase() {
	UpdateStageObjects();

	// デスパーティクルの更新
	if (deathParticles_) {
		deathParticles_->Update();
	}

	// カメラの更新
	UpdateDebugCamera();
}

void GameScene::Draw() {
	Model::PreDraw();
	cityBackground_->Draw();

	if (phase_ == Phase::kPlay || phase_ == Phase::kFadeIn || (phase_ == Phase::kFadeOut && isCleared_)) {
		player_->Draw();
	}

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			modelBlock_->Draw(*worldTransformBlock, camera_);
		}
	}

	if (checkpoint_) {
		checkpoint_->Draw();
	}

	for (Coin* coin : coins_) {
		coin->Draw();
	}

	goalStar_->Draw();

	// 煙は半透明なので、背後のブロックが透けて見えるようにブロックの後に描画する
	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}

	// 半透明のパーティクルは最後にまとめて描画する
	particles_.Draw();

	if (deathParticles_) {
		deathParticles_->Draw();
	}
	Model::PostDraw();

	DrawHud();

	if (fade_) {
		fade_->Draw();
	}
}

void GameScene::DrawHud() {
	constexpr float kCharSize = 32.0f;
	constexpr float kLabelY = 16.0f;
	constexpr float kValueY = 52.0f;
	const Vector4 kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
	const Vector4 kYellow = {1.0f, 0.9f, 0.3f, 1.0f};
	const Vector4 kRed = {1.0f, 0.3f, 0.25f, 1.0f};

	char buffer[32];

	text_.Clear();

	text_.Print("SCORE", 40.0f, kLabelY, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%06u", session_->score);
	text_.Print(buffer, 40.0f, kValueY, kCharSize);

	text_.Print("COIN", 330.0f, kLabelY, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "*%02u", session_->coins);
	text_.Print(buffer, 330.0f, kValueY, kCharSize, kYellow);

	text_.Print("STAGE", 560.0f, kLabelY, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "%u", session_->stageIndex + 1);
	text_.Print(buffer, 560.0f, kValueY, kCharSize);

	text_.Print("LIFE", 800.0f, kLabelY, kCharSize);
	std::snprintf(buffer, sizeof(buffer), "*%d", session_->lives);
	text_.Print(buffer, 800.0f, kValueY, kCharSize);

	// 残り時間が少なくなったら赤くする
	text_.Print("TIME", 1240.0f, kLabelY, kCharSize, kWhite, BitmapText::Align::kRight);
	std::snprintf(buffer, sizeof(buffer), "%03d", static_cast<int>(std::ceil(remainingTime_)));
	text_.Print(buffer, 1240.0f, kValueY, kCharSize, remainingTime_ <= kHurryTime ? kRed : kWhite, BitmapText::Align::kRight);

	// 得点表示（だんだん薄くなる）
	for (const ScorePopup& popup : popups_) {
		const Vector2 screen = WorldToScreen(popup.position);
		const float alpha = std::clamp(popup.timer / kPopupDuration * 2.0f, 0.0f, 1.0f);
		text_.Print(popup.text, screen.x, screen.y, 24.0f, {1.0f, 1.0f, 1.0f, alpha}, BitmapText::Align::kCenter);
	}

	text_.Draw();
}

Vector2 GameScene::WorldToScreen(const Vector3& position) const {
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;

	const Matrix4x4 viewProjection = calc::Multiply(camera_.matView, camera_.matProjection);
	const Vector3 ndc = calc::Transform(position, viewProjection);

	return {(ndc.x + 1.0f) * 0.5f * kScreenWidth, (1.0f - ndc.y) * 0.5f * kScreenHeight};
}

void GameScene::AddPopup(const Vector3& position, const std::string& text) { popups_.push_back({position, text, kPopupDuration}); }

void GameScene::AddStompScore(const Vector3& position) {
	++stompCombo_;

	const size_t numScores = std::size(kStompScores);
	if (stompCombo_ <= numScores) {
		const uint32_t score = kStompScores[stompCombo_ - 1];
		session_->AddScore(score);
		AddPopup(position, std::to_string(score));
	} else {
		// 得点表を超えて踏み続けたら1UP
		session_->AddLife();
		AddPopup(position, "1UP");
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
			particles_.EmitStompImpact(calc::Add(enemy->GetWorldPosition(), Vector3(0.0f, 0.3f, 0.0f)));
			++defeatedEnemies_;
			AddStompScore(enemy->GetWorldPosition());

		} else {
			// 横からの接触、または無敵敵への攻撃
			player_->OnCollision(enemy);
		}
	}

	// コインの取得
	for (Coin* coin : coins_) {
		if (!coin->IsAvailable()) {
			continue;
		}
		if (!calc::IsCollision(playerAABB, coin->GetAABB())) {
			continue;
		}

		coin->Collect();
		{
			const AABB coinAABB = coin->GetAABB();
			const Vector3 coinCenter = {(coinAABB.min.x + coinAABB.max.x) * 0.5f, (coinAABB.min.y + coinAABB.max.y) * 0.5f, 0.0f};
			particles_.EmitSparkles(coinCenter, {1.0f, 0.85f, 0.25f, 1.0f}, 10, 0.08f);
		}
		++stageCoins_;
		session_->AddScore(kCoinScore);
		// 100枚集めたら1UP
		if (session_->AddCoin()) {
			AddPopup(player_->GetWorldPosition(), "1UP");
		}
	}
}

GameScene::~GameScene() {
	delete model_;
	delete player_;
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	for (Coin* coin : coins_) {
		delete coin;
	}
	delete checkpoint_;
	delete modelBlock_;
	delete cityBackground_;
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
	delete modelSuitman_;
	delete modelSuitmanBody_;
	delete modelSuitmanArm_;
	delete modelSuitmanLeg_;
	delete modelInvincibleEnemy_;
	delete modelDeathParticle_;
	delete goalStar_;
	delete modelGoalStar_;
	delete modelSmoke_;
	delete modelCoin_;
	delete modelCheckpoint_;
	delete modelPlayerBody_;
	delete modelPlayerArm_;
	delete modelPlayerLeg_;
	delete modelPlayerScarf_;
}

bool GameScene::IsGoalReached() {
	if (goalStar_->IsCollected()) {
		return false;
	}

	// プレイヤーが星に触れたらゴール
	return calc::IsCollision(player_->GetAABB(), goalStar_->GetAABB());
}
