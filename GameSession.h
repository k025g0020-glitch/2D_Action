#pragma once
#include <array>
#include <cstdint>

// ステージ情報（ステージを増やす時はここに追加する）
struct StageInfo {
	const char* csvPath;        // マップCSVのパス
	uint32_t timeLimit;         // 制限時間（秒）
	const char* name;           // ステージ紹介画面に出す街の名前（英大文字・数字・記号のみ）
	const char* hint = nullptr; // ステージ紹介画面に出すヒント（なくてもよい）
};

// 夜の街のビル群を駆け抜けて、空の星を目指す
inline constexpr std::array<StageInfo, 3> kStages = {{
    {"Resources/stage1.csv", 240, "DOWNTOWN", "HOLD SHIFT AND JUMP 3 TIMES IN A ROW!"},
    {"Resources/stage2.csv", 270, "ROOFTOPS", "TRIPLE JUMP UP THE ROOFTOP STAIRS!"},
    {"Resources/stage3.csv", 300, "SKYSCRAPERS", "DASH ACROSS THE ROOFTOPS. DON'T FALL!"},
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
