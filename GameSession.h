#pragma once
#include <array>
#include <cstdint>

// ステージ情報（ステージを増やす時はここに追加する）
struct StageInfo {
	const char* csvPath;        // マップCSVのパス
	uint32_t timeLimit;         // 制限時間（秒）
	const char* hint = nullptr; // ステージ紹介画面に出すヒント（なくてもよい）
};

// 夜の街のビル群を駆け抜けて、空の星を目指す
inline constexpr std::array<StageInfo, 3> kStages = {{
    {"Resources/stage1.csv", 240, "SHIFTを押しながら 3回続けてジャンプしよう！"},
    {"Resources/stage2.csv", 270, "屋上の階段を 三段ジャンプで駆け上がれ！"},
    {"Resources/stage3.csv", 300, "ダッシュで屋上を駆け抜けろ。落ちるな！"},
}};

// クリアしたステージの結果
struct StageResult {
	uint32_t coins = 0;           // このステージで取ったコイン数
	uint32_t defeatedEnemies = 0; // 倒した敵の数
	uint32_t remainingTime = 0;   // 残り時間（秒）
};

// シーンをまたいで引き継ぐゲーム全体の状態（残機・スコアなど）
struct GameSession {
	static inline const int32_t kInitialLives = 3;
	static inline const int32_t kMaxLives = 99;
	static inline const uint32_t kMaxScore = 999999;
	// コインを何枚集めると1UPするか
	static inline const uint32_t kCoinsForExtraLife = 100;

	int32_t lives = kInitialLives;
	uint32_t score = 0;
	uint32_t coins = 0;
	uint32_t stageIndex = 0;
	// 現在のステージで通過した一番先の中間ポイントの番号（左から 0, 1, 2…。通過していなければ -1）
	int32_t checkpointIndex = -1;
	bool HasCheckpoint() const { return checkpointIndex >= 0; }
	// ミスした後の再挑戦か（復活直後は少しの間無敵にする）
	bool isRespawn = false;

	StageResult lastStageResult;

	// ステージを始めた時の残機・スコア・コイン（再挑戦した時にこの値に戻す）
	struct StageStart {
		int32_t lives = kInitialLives;
		uint32_t score = 0;
		uint32_t coins = 0;
	};
	StageStart stageStart;

	void Reset() { *this = GameSession(); }

	// 新しいステージを始める時に呼ぶ（中間ポイントを消し、再挑戦用に今の状態を覚えておく）
	void StartNewStage() {
		checkpointIndex = -1;
		isRespawn = false;
		stageStart = {lives, score, coins};
	}

	// 同じステージを最初からやり直す。スコアとコインはステージを始めた時に戻す
	// 残機は、ゲームオーバーからなら最初の数に戻し、それ以外はステージを始めた時より増やさない
	// （クリアした後の再挑戦で、1UP やスコアを何度も稼げないようにする）
	void RestartStage(bool fromGameOver) {
		score = stageStart.score;
		coins = stageStart.coins;
		lives = fromGameOver ? kInitialLives : (lives < stageStart.lives ? lives : stageStart.lives);
		if (lives <= 0) {
			lives = 1;
		}
		stageStart.lives = lives;
		checkpointIndex = -1;
		isRespawn = false;
	}

	const StageInfo& GetStage() const { return kStages[stageIndex]; }
	bool IsLastStage() const { return stageIndex + 1 >= kStages.size(); }

	void AddScore(uint32_t value) {
		score += value;
		if (score > kMaxScore) {
			score = kMaxScore;
		}
	}

	void AddLife() {
		if (lives < kMaxLives) {
			++lives;
		}
	}

	// コインを1枚加算する。1UPした場合は true を返す
	bool AddCoin() {
		++coins;
		if (coins >= kCoinsForExtraLife) {
			coins -= kCoinsForExtraLife;
			AddLife();
			return true;
		}
		return false;
	}
};
