#pragma once
#include "KamataEngine.h"
#include <array>
#include <cstdint>

// 効果音（Resources/sound/ の WAV）をまとめて読み込んで鳴らす
// 起動時に一度だけ Load() を呼び、鳴らしたい所で Play() を呼ぶ
class SoundEffects {
public:
	enum class Id {
		kJump1,        // 1段目のジャンプ
		kJump2,        // 2段目のジャンプ
		kJump3,        // 3段目のジャンプ
		kCoin,         // コインを取った
		kStomp,        // 敵を踏んだ
		kOneUp,        // 1UP
		kMiss,         // ミス
		kCheckpoint,   // 中間ポイントを通過した
		kGoal,         // ゴールの星を取った
		kHurry,        // 残り時間が少なくなった
		kRobotCharge,  // 警備ロボットが武器を構えた
		kRobotThrow,   // 警備ロボットが武器を投げた
		kGrenadeBurst, // 電撃グレネードがはじけた
		kDecide,       // タイトル・リザルトの決定
		kCount,
	};

	static void Load();
	static void Play(Id id);

private:
	struct Entry {
		const char* fileName;
		// 鳴らす時の音量（ファイルは全部同じ大きさにそろえてあるので、ここで聞こえ方を調整する）
		float volume;
	};

	// Id と同じ順番に並べる
	static inline const Entry kEntries[] = {
	    {"sound/jump1.wav", 0.35f},
	    {"sound/jump2.wav", 0.35f},
	    {"sound/jump3.wav", 0.4f},
	    {"sound/coin.wav", 0.4f},
	    {"sound/stomp.wav", 0.6f},
	    {"sound/oneup.wav", 0.6f},
	    {"sound/miss.wav", 0.7f},
	    {"sound/checkpoint.wav", 0.6f},
	    {"sound/goal.wav", 0.7f},
	    {"sound/hurry.wav", 0.6f},
	    {"sound/robot_charge.wav", 0.45f},
	    {"sound/robot_throw.wav", 0.5f},
	    {"sound/grenade_burst.wav", 0.6f},
	    {"sound/decide.wav", 0.6f},
	};
	static_assert(std::size(kEntries) == static_cast<size_t>(Id::kCount), "kEntries と Id の数をそろえる");

	static inline std::array<uint32_t, static_cast<size_t>(Id::kCount)> handles_ = {};
	static inline bool isLoaded_ = false;
};
