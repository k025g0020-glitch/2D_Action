#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"
#include "GameSession.h"
#include "BgmPlayer.h"
#include "ResultScene.h"
#include "SoundEffects.h"
#include "SoundSettings.h"
#include "StageIntroScene.h"
#include "TitleScene.h"

using namespace KamataEngine;

enum class Scene {
	kUnknown = 0,
	kTitle,
	kStageIntro,
	kGame,
	kResult,
};

Scene scene = Scene::kUnknown;

// 残機・スコア・現在のステージなど、シーンをまたいで引き継ぐ情報
GameSession gameSession;

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;
StageIntroScene* stageIntroScene = nullptr;
ResultScene* resultScene = nullptr;

// プロトタイプ宣言
void ChangeScene();
void UpdateScene();
void DrawScene();
void StartTitle();
void StartStageIntro();
void StartGame();
void StartResult(ResultScene::Type type);

void StartTitle() {
	scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize();
}

void StartStageIntro() {
	scene = Scene::kStageIntro;
	stageIntroScene = new StageIntroScene();
	stageIntroScene->Initialize(&gameSession);
}

void StartGame() {
	scene = Scene::kGame;
	gameScene = new GameScene();
	gameScene->Initialize(&gameSession);
}

void StartResult(ResultScene::Type type) {
	scene = Scene::kResult;
	resultScene = new ResultScene();
	resultScene->Initialize(&gameSession, type);
}

// シーン切り替え処理
// タイトル → ステージ紹介 → ゲーム → (クリア) リザルト → 次のステージ紹介 …
//                                   → (ミス) 残機があればステージ紹介、なければゲームオーバー
void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene->IsFinished()) {
			delete titleScene;
			titleScene = nullptr;
			// 新しくゲームを始める
			gameSession.Reset();
			gameSession.StartNewStage();
			StartStageIntro();
		}
		break;

	case Scene::kStageIntro:
		if (stageIntroScene->IsFinished()) {
			delete stageIntroScene;
			stageIntroScene = nullptr;
			StartGame();
		}
		break;

	case Scene::kGame:
		if (gameScene->IsFinished()) {
			const GameScene::Result result = gameScene->GetResult();
			delete gameScene;
			gameScene = nullptr;

			if (result == GameScene::Result::kCleared) {
				StartResult(ResultScene::Type::kStageClear);
			} else if (result == GameScene::Result::kRetry) {
				// ポーズから再挑戦：同じステージを最初から
				gameSession.RestartStage(false);
				StartStageIntro();
			} else if (result == GameScene::Result::kQuit) {
				// ポーズから終了：タイトルに戻る
				StartTitle();
			} else {
				// ミスしたら残機を減らす
				--gameSession.lives;
				if (gameSession.lives > 0) {
					gameSession.isRespawn = true;
					StartStageIntro();
				} else {
					StartResult(ResultScene::Type::kGameOver);
				}
			}
		}
		break;

	case Scene::kResult:
		if (resultScene->IsFinished()) {
			const ResultScene::Choice choice = resultScene->GetChoice();
			delete resultScene;
			resultScene = nullptr;

			// 結果画面で選んだことに合わせて、次のステージ・再挑戦・タイトルへ
			switch (choice) {
			case ResultScene::Choice::kNextStage:
				++gameSession.stageIndex;
				gameSession.StartNewStage();
				StartStageIntro();
				break;
			case ResultScene::Choice::kRetry:
				// ゲームオーバーからの再挑戦は残機を最初の数に戻す
				gameSession.RestartStage(gameSession.lives <= 0);
				StartStageIntro();
				break;
			case ResultScene::Choice::kQuit:
				StartTitle();
				break;
			}
		}
		break;

	default:
		break;
	}
}

// 現在シーンの更新処理
void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kStageIntro:
		stageIntroScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	case Scene::kResult:
		resultScene->Update();
		break;
	default:
		break;
	}
}

// 現在シーンの描画処理
void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kStageIntro:
		stageIntroScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	case Scene::kResult:
		resultScene->Draw();
		break;
	default:
		break;
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	// ウィンドウのタイトル
	Initialize(L"星屑ランナー");

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// 保存しておいた音量を読み込み、BGM と効果音を読み込んでおく
	SoundSettings::Load();
	BgmPlayer::Load();
	SoundEffects::Load();

	StartTitle();

	while (true) {
		if (Update()) {
			break;
		}

		// シーン切り替え
		ChangeScene();
		// 現在シーン更新
		UpdateScene();

		dxCommon->PreDraw();

		// 現在シーンの描画
		DrawScene();

		dxCommon->PostDraw();
	}

	Finalize();

	delete titleScene;
	titleScene = nullptr;
	delete stageIntroScene;
	stageIntroScene = nullptr;
	delete gameScene;
	gameScene = nullptr;
	delete resultScene;
	resultScene = nullptr;

	return 0;
}
