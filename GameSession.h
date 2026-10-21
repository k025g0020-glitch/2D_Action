#pragma once
#include <array>
#include <cstdint>

// ステージ情報（ステージを増やす時はここに追加する）
struct StageInfo {
	const char* csvPath; // マップCSVのパス
	uint32_t timeLimit;  // 制限時間（秒）
};

inline constexpr std::array<StageInfo, 2> kStages = {{
    {"Resources/stage1.csv", 120},
    {"Resources/stage2.csv", 150},
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
	// 現在のステージで中間ポイントを通過したか
	bool checkpointReached = false;
	// ミスした後の再挑戦か（復活直後は少しの間無敵にする）
	bool isRespawn = false;

	StageResult lastStageResult;

	void Reset() { *this = GameSession(); }

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
