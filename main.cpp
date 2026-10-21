#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"
#include "GameSession.h"
#include "ResultScene.h"
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
			} else {
				// ミスしたら残機を減らす
				--gameSession.lives;
				if (gameSession.lives > 0) {
					StartStageIntro();
				} else {
					StartResult(ResultScene::Type::kGameOver);
				}
			}
		}
		break;

	case Scene::kResult:
		if (resultScene->IsFinished()) {
			delete resultScene;
			resultScene = nullptr;

			// ゲームオーバーか最終ステージクリアならタイトルへ、それ以外は次のステージへ
			if (gameSession.lives <= 0 || gameSession.IsLastStage()) {
				StartTitle();
			} else {
				++gameSession.stageIndex;
				gameSession.checkpointReached = false;
				StartStageIntro();
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
	
	Initialize(L"LE2D_20_タキエ_コウガ_AL3");

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

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
