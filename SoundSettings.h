#pragma once

// BGM と効果音の音量（0〜10 の11段階）。タイトル画面の SOUND で変えられる
// 変えた音量はファイルに保存し、次に起動した時も同じ音量にする
class SoundSettings {
public:
	static inline const int kMaxLevel = 10;

	// 保存しておいた音量を読み込む（ファイルがなければ最大のまま）
	static void Load();
	static void Save();

	static int GetBgmLevel() { return bgmLevel_; }
	static int GetSeLevel() { return seLevel_; }
	static void SetBgmLevel(int level);
	static void SetSeLevel(int level);

	// 再生する時に音量に掛ける割合（0.0〜1.0）
	static float GetBgmScale() { return static_cast<float>(bgmLevel_) / kMaxLevel; }
	static float GetSeScale() { return static_cast<float>(seLevel_) / kMaxLevel; }

private:
	// 保存先（実行した時のフォルダ）
	static inline const char* kFileName = "sound_settings.txt";

	static inline int bgmLevel_ = kMaxLevel;
	static inline int seLevel_ = kMaxLevel;
};
