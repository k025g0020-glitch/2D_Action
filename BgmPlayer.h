#pragma once
#include "KamataEngine.h"
#include <array>
#include <cstdint>

// BGM（Resources/bgm/ の WAV）をループ再生する。同時に鳴るのは1曲だけ
// 起動時に一度だけ Load() を呼び、シーンの始めに Play() / Stop() を呼ぶ
class BgmPlayer {
public:
	enum class Id {
		kTitle,  // タイトル
		kStage1, // ステージ1
		kStage2, // ステージ2
		kStage3,   // ステージ3
		kHurry,    // 残り時間が少なくなった時（ステージの曲から切り替える）
		kResult,   // ステージクリアのリザルト
		kAllClear, // 最後のステージをクリアした時のリザルト
		kGameOver, // ゲームオーバー
		kCount,
		kNone = kCount,
	};

	static void Load();
	// 曲を流す（同じ曲が流れている時はそのまま続ける）
	static void Play(Id id);
	static void Stop();
	// ステージ番号（0 から）に合った曲
	static Id StageBgm(uint32_t stageIndex);

private:
	struct Entry {
		const char* fileName;
		// 曲ごとの大きさの違いをならし、効果音が聞こえるくらいに抑えた音量
		float volume;
	};

	// Id と同じ順番に並べる
	static inline const Entry kEntries[] = {
	    {"bgm/title.wav", 0.6f},
	    {"bgm/stage1.wav", 0.39f},
	    {"bgm/stage2.wav", 0.34f},
	    {"bgm/stage3.wav", 0.32f},
	    {"bgm/hurry.wav", 0.23f},
	    {"bgm/result.wav", 0.35f},
	    {"bgm/allclear.wav", 0.16f},
	    {"bgm/gameover.wav", 0.36f},
	};
	static_assert(std::size(kEntries) == static_cast<size_t>(Id::kCount), "kEntries と Id の数をそろえる");

	static inline std::array<uint32_t, static_cast<size_t>(Id::kCount)> handles_ = {};
	static inline bool isLoaded_ = false;
	// 今流れている曲と、その再生の番号（止める時に使う）
	static inline Id current_ = Id::kNone;
	static inline uint32_t voiceHandle_ = 0;
};
