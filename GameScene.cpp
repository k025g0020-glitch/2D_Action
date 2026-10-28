#define NOMINMAX
#include "GameScene.h"
#include "BgmPlayer.h"
#include "SoundEffects.h"
#include "SoundSettings.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iterator>

using namespace KamataEngine;

void GameScene::Initialize(GameSession* session) {
	assert(session);
	session_ = session;

	BgmPlayer::Play(BgmPlayer::StageBgm(session_->stageIndex));

	// プレイヤー：フードとバイザーの、街を駆けるランナー
	model_ = Model::CreateFromOBJ("runner", true);
	modelSuitman_ = Model::CreateFromOBJ("suitman", true);
	modelSuitmanBody_ = Model::CreateFromOBJ("suitman_body", true);
	modelSuitmanArm_ = Model::CreateFromOBJ("suitman_arm", true);
	modelSuitmanLeg_ = Model::CreateFromOBJ("suitman_leg", true);
	modelDrone_ = Model::CreateFromOBJ("drone", true);
	modelDroneBody_ = Model::CreateFromOBJ("drone_body", true);
	modelDroneRotor_ = Model::CreateFromOBJ("drone_rotor", true);
	modelInvincibleEnemy_ = Model::CreateFromOBJ("robot", true);
	modelRobotBody_ = Model::CreateFromOBJ("robot_body", true);
	modelRobotHead_ = Model::CreateFromOBJ("robot_head", true);
	modelRobotBeacon_ = Model::CreateFromOBJ("robot_beacon", true);
	modelGrenade_ = Model::CreateFromOBJ("grenade", true);
	// 足場：街の屋上らしいコンクリートと鉄骨のブロック
	modelBlock_ = Model::CreateFromOBJ("cityblock", true);
	modelDeathParticle_ = Model::CreateFromOBJ("deathParticle", true);
	modelGoalStar_ = Model::CreateFromOBJ("star", true);
	modelBeam_ = Model::CreateFromOBJ("beam", true);
	modelSmoke_ = Model::CreateFromOBJ("smoke", true);
	modelCoin_ = Model::CreateFromOBJ("coin", true);
	modelSpark_ = Model::CreateFromOBJ("spark", true);
	modelCheckpoint_ = Model::CreateFromOBJ("checkpoint", true);
	modelCheckpointFlag_ = Model::CreateFromOBJ("checkpoint_flag", true);
	modelPlayerBody_ = Model::CreateFromOBJ("runner_body", true);
	modelPlayerArm_ = Model::CreateFromOBJ("runner_arm", true);
	modelPlayerLeg_ = Model::CreateFromOBJ("runner_leg", true);
	modelPlayerScarf_ = Model::CreateFromOBJ("runner_scarf", true);

	camera_.Initialize();
	particles_.Initialize(modelSmoke_, modelSpark_, &camera_);
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

	// 一時停止の画面
	isPaused_ = false;
	pauseMenu_.Initialize({
	    {VolumeMenu::RowType::kAction, "RESUME"},
	    {VolumeMenu::RowType::kAction, "RETRY"},
	    {VolumeMenu::RowType::kAction, "QUIT"},
	    {VolumeMenu::RowType::kBgm, "BGM"},
	    {VolumeMenu::RowType::kSe, "SE"},
	});
	pauseText_.Initialize(320);
	pauseOverlay_ = Sprite::Create(TextureManager::Load("white1x1.png"), {0.0f, 0.0f});
	pauseOverlay_->SetSize({1280.0f, 720.0f});
	pauseOverlay_->SetColor({0.0f, 0.0f, 0.0f, 0.6f});

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);

	phase_ = Phase::kFadeIn;
	result_ = Result::kNone;
}

void GameScene::SpawnObjectsFromMap() {
	Vector3 playerInitialPosition = mapChipField_->GetMapChipPositionByIndex(2, 18);
	std::vector<Vector3> checkpointPositions;

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
					// ゴール側からスタート側へ飛ぶ敵：監視ドローン
					enemy->Initialize(modelDrone_, &camera_, mapChipField_, position, Enemy::Type::kFlying, modelSmoke_);
					enemy->SetDroneModels(modelDroneBody_, modelDroneRotor_);
				} else {
					// 動かない無敵敵：装甲の警備ロボット
					enemy->Initialize(modelInvincibleEnemy_, &camera_, mapChipField_, position, Enemy::Type::kInvincible);
					enemy->SetRobotModels(modelRobotBody_, modelRobotHead_, modelRobotBeacon_);
					enemy->SetWeaponModel(modelGrenade_);
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
				goalStar_->SetBeamModel(modelBeam_);
				break;
			}

			case MapChipType::kCheckpoint:
				checkpointPositions.push_back(position);
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
		goalStar_->SetBeamModel(modelBeam_);
	}

	// 中間ポイントは左から順に番号を付ける。通過済みのものは最初から光らせる
	std::sort(checkpointPositions.begin(), checkpointPositions.end(), [](const Vector3& a, const Vector3& b) { return a.x < b.x; });
	for (size_t i = 0; i < checkpointPositions.size(); ++i) {
		Checkpoint* checkpoint = new Checkpoint();
		checkpoint->Initialize(modelCheckpoint_, &camera_, checkpointPositions[i], static_cast<int32_t>(i) <= session_->checkpointIndex);
		checkpoint->SetFlagModel(modelCheckpointFlag_);
		checkpoints_.push_back(checkpoint);
	}

	// 通過した一番先の中間ポイントから再開する
	if (session_->HasCheckpoint() && static_cast<size_t>(session_->checkpointIndex) < checkpoints_.size()) {
		playerInitialPosition = checkpoints_[session_->checkpointIndex]->GetPosition();
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
	// 遊んでいる間は ESC で一時停止できる。一時停止中はゲームを止めて、メニューだけ動かす
	if (phase_ == Phase::kPlay && UpdatePause()) {
		return;
	}

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
	BgmPlayer::Stop();
	SoundEffects::Play(SoundEffects::Id::kMiss);
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
		// 段が上がるほど高い音
		static const SoundEffects::Id kJumpSounds[] = {SoundEffects::Id::kJump1, SoundEffects::Id::kJump2, SoundEffects::Id::kJump3};
		const uint32_t stage = std::clamp(player_->GetJumpStage(), 1u, 3u);
		SoundEffects::Play(kJumpSounds[stage - 1]);
		particles_.EmitJumpDust(feet, player_->GetJumpStage());
		// 3段目は星の輪
		if (player_->GetJumpStage() == 3) {
			particles_.EmitStarRing(player_->GetWorldPosition());
		}
	}

	if (player_->HasLanded()) {
		particles_.EmitLandingDust(feet, player_->GetLandingSpeed());
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
		if (enemy->HasStartedWindUp()) {
			SoundEffects::Play(SoundEffects::Id::kRobotCharge);
		}
		// 警備ロボットが武器を投げた
		if (enemy->HasThrown()) {
			SoundEffects::Play(SoundEffects::Id::kRobotThrow);
			ShockGrenade* grenade = new ShockGrenade();
			grenade->Initialize(modelGrenade_, &camera_, mapChipField_, enemy->GetThrowOrigin(), enemy->GetThrowTarget());
			grenades_.push_back(grenade);
		}
	}

	// 電撃グレネードの更新。ブロックに当たったらはじけて消える
	for (ShockGrenade* grenade : grenades_) {
		// プレイヤーに当たってはじけたものは、当たった時に演出を出している
		const bool wasFlying = !grenade->IsExploded() && !grenade->IsGone();
		grenade->Update();
		if (!wasFlying) {
			continue;
		}
		if (grenade->IsExploded()) {
			SoundEffects::Play(SoundEffects::Id::kGrenadeBurst);
			particles_.EmitShockBurst(grenade->GetPosition());
		} else if (!grenade->IsGone() && effectFrame_ % kShockTrailInterval == 0) {
			particles_.EmitShockTrail(grenade->GetPosition());
		}
	}
	grenades_.remove_if([](ShockGrenade* grenade) {
		if (grenade->IsExploded() || grenade->IsGone()) {
			delete grenade;
			return true;
		}
		return false;
	});

	for (Coin* coin : coins_) {
		coin->Update();
	}

	for (Checkpoint* checkpoint : checkpoints_) {
		checkpoint->Update();
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
		} else {
			if (effectFrame_ % kTwinkleInterval == 0) {
				particles_.EmitTwinkle(starCenter, 0.5f);
			}
			// 光の柱の中を光の粒が昇っていく
			if (effectFrame_ % kRisingLightInterval == 0) {
				particles_.EmitRisingLight(goalStar_->GetBasePosition(), goalStar_->GetBeamHalfWidth());
			}
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

	// 警備ロボットにプレイヤーの位置を教える（近くにいれば武器を投げてくる）
	for (Enemy* enemy : enemies_) {
		enemy->SetThrowTarget(player_->GetWorldPosition());
	}

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
	for (size_t i = 0; i < checkpoints_.size(); ++i) {
		Checkpoint* checkpoint = checkpoints_[i];
		if (checkpoint->IsActivated() || !checkpoint->IsPassedBy(player_->GetWorldPosition())) {
			continue;
		}
		checkpoint->Activate();
		SoundEffects::Play(SoundEffects::Id::kCheckpoint);
		session_->checkpointIndex = (std::max)(session_->checkpointIndex, static_cast<int32_t>(i));
		// 旗の先から紙吹雪
		particles_.EmitConfetti(calc::Add(checkpoint->GetPosition(), Vector3(0.4f, 1.3f, 0.0f)));
		AddPopup(calc::Add(checkpoint->GetPosition(), Vector3(0.0f, 2.0f, 0.0f)), "CHECKPOINT");
	}

	// 穴に落ちたらミス
	if (player_->GetWorldPosition().y < kFallDeathY) {
		player_->Die();
	}

	// 残り時間を減らし、0になったらミス。少なくなった時に一度だけ警告音を鳴らす
	const bool wasHurry = remainingTime_ <= kHurryTime;
	remainingTime_ = std::max(remainingTime_ - 1.0f / 60.0f, 0.0f);
	if (!wasHurry && remainingTime_ <= kHurryTime) {
		SoundEffects::Play(SoundEffects::Id::kHurry);
		// 曲も緊迫したものに切り替える
		BgmPlayer::Play(BgmPlayer::Id::kHurry);
	}
	if (remainingTime_ <= 0.0f) {
		player_->Die();
	}

	// 星を獲得したらゴール
	if (!player_->IsDead() && IsGoalReached()) {
		goalStar_->Collect();
		BgmPlayer::Stop();
		SoundEffects::Play(SoundEffects::Id::kGoal);
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

	// ブロックはカメラに映っている列だけ描く（横に長いステージでも重くならないように）
	const float viewHalfWidth = cameraController_->GetViewHalfWidth() + 1.0f;
	const int32_t numColumns = static_cast<int32_t>(mapChipField_->GetNumBlockHorizontal());
	const int32_t firstColumn = (std::max)(0, static_cast<int32_t>(std::floor(camera_.translation_.x - viewHalfWidth)));
	const int32_t lastColumn = (std::min)(numColumns - 1, static_cast<int32_t>(std::ceil(camera_.translation_.x + viewHalfWidth)));
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (int32_t x = firstColumn; x <= lastColumn; ++x) {
			if (WorldTransform* worldTransformBlock = worldTransformBlockLine[x]) {
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
	}

	for (Checkpoint* checkpoint : checkpoints_) {
		checkpoint->Draw();
	}

	for (Coin* coin : coins_) {
		coin->Draw();
	}

	goalStar_->Draw();

	// 煙は半透明なので、背後のブロックが透けて見えるようにブロックの後に描画する
	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}

	for (ShockGrenade* grenade : grenades_) {
		grenade->Draw();
	}

	// 半透明のパーティクルは最後にまとめて描画する
	particles_.Draw();

	if (deathParticles_) {
		deathParticles_->Draw();
	}
	Model::PostDraw();

	DrawHud();
	DrawPause();

	if (fade_) {
		fade_->Draw();
	}
}

bool GameScene::UpdatePause() {
	// ポーズのメニューの行の番号（Initialize で並べた順）
	enum PauseRow { kResume, kRetry, kQuit };

	Input* input = Input::GetInstance();
	if (!isPaused_) {
		if (!input->TriggerKey(DIK_ESCAPE)) {
			return false;
		}
		isPaused_ = true;
		pauseBlinkTimer_ = 0.0f;
		// RESUME を選んだ状態で開き、すぐ戻れるようにする
		pauseMenu_.Open(kResume);
		SoundEffects::Play(SoundEffects::Id::kDecide);
		return true;
	}

	pauseBlinkTimer_ += 1.0f / 60.0f;
	const int selected = pauseMenu_.Update();
	// RESUME を選ぶか、もう一度 ESC を押したらゲームに戻る
	if (selected == kResume || input->TriggerKey(DIK_ESCAPE)) {
		SoundSettings::Save();
		isPaused_ = false;
	} else if (selected == kRetry || selected == kQuit) {
		// 再挑戦・終了は、フェードアウトしてからシーンを終える
		isPaused_ = false;
		result_ = (selected == kRetry) ? Result::kRetry : Result::kQuit;
		BgmPlayer::Stop();
		phase_ = Phase::kFadeOut;
		fade_->Start(Fade::Status::FadeOut, kFadeDuration);
	}
	return true;
}

void GameScene::DrawPause() {
	if (!isPaused_) {
		return;
	}
	Sprite::PreDraw();
	pauseOverlay_->Draw();
	Sprite::PostDraw();

	pauseText_.Clear();
	pauseMenu_.Draw(pauseText_, "PAUSE", 150.0f, pauseBlinkTimer_);
	pauseText_.Draw();
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
		SoundEffects::Play(SoundEffects::Id::kStomp);
	} else {
		// 得点表を超えて踏み続けたら1UP
		SoundEffects::Play(SoundEffects::Id::kOneUp);
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

		const bool stompedFromAbove = player_->GetVelocity().y < 0.0f && playerCenterY > enemyCenterY + 0.4f;

		if (stompedFromAbove && enemy->CanBeDefeated()) {

			enemy->Defeat();
			player_->BounceFromEnemy();
			particles_.EmitStompImpact(calc::Add(enemy->GetWorldPosition(), Vector3(0.0f, 0.6f, 0.0f)));
			++defeatedEnemies_;
			AddStompScore(enemy->GetWorldPosition());

		} else {
			// 横からの接触、または無敵敵への攻撃
			player_->OnCollision(enemy);
		}
	}

	// 電撃グレネードに当たるとミス（無敵中でもグレネードははじけて消える）
	for (ShockGrenade* grenade : grenades_) {
		if (grenade->IsExploded() || grenade->IsGone() || !calc::IsCollision(playerAABB, grenade->GetAABB())) {
			continue;
		}
		grenade->Explode();
		SoundEffects::Play(SoundEffects::Id::kGrenadeBurst);
		particles_.EmitShockBurst(grenade->GetPosition());
		player_->OnCollision(nullptr);
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
		SoundEffects::Play(SoundEffects::Id::kCoin);
		{
			const AABB coinAABB = coin->GetAABB();
			const Vector3 coinCenter = {(coinAABB.min.x + coinAABB.max.x) * 0.5f, (coinAABB.min.y + coinAABB.max.y) * 0.5f, 0.0f};
			particles_.EmitSparkles(coinCenter, {1.0f, 0.85f, 0.25f, 1.0f}, 10, 0.08f);
		}
		++stageCoins_;
		session_->AddScore(kCoinScore);
		// 100枚集めたら1UP
		if (session_->AddCoin()) {
			SoundEffects::Play(SoundEffects::Id::kOneUp);
			AddPopup(player_->GetWorldPosition(), "1UP");
		}
	}
}

GameScene::~GameScene() {
	delete pauseOverlay_;
	delete model_;
	delete player_;
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	for (ShockGrenade* grenade : grenades_) {
		delete grenade;
	}
	for (Coin* coin : coins_) {
		delete coin;
	}
	for (Checkpoint* checkpoint : checkpoints_) {
		delete checkpoint;
	}
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
	delete modelSuitman_;
	delete modelSuitmanBody_;
	delete modelSuitmanArm_;
	delete modelSuitmanLeg_;
	delete modelDrone_;
	delete modelDroneBody_;
	delete modelDroneRotor_;
	delete modelInvincibleEnemy_;
	delete modelRobotBody_;
	delete modelRobotHead_;
	delete modelRobotBeacon_;
	delete modelGrenade_;
	delete modelDeathParticle_;
	delete goalStar_;
	delete modelGoalStar_;
	delete modelBeam_;
	delete modelSmoke_;
	delete modelCoin_;
	delete modelSpark_;
	delete modelCheckpoint_;
	delete modelCheckpointFlag_;
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
