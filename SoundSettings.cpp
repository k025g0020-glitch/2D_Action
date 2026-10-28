#include "SoundSettings.h"
#include <algorithm>
#include <fstream>
#include <string>

void SoundSettings::Load() {
	std::ifstream file(kFileName);
	std::string key;
	int level = 0;
	// 「bgm 8」「se 6」のような行を読む
	while (file >> key >> level) {
		if (key == "bgm") {
			SetBgmLevel(level);
		} else if (key == "se") {
			SetSeLevel(level);
		}
	}
}

void SoundSettings::Save() {
	std::ofstream file(kFileName);
	file << "bgm " << bgmLevel_ << "\n";
	file << "se " << seLevel_ << "\n";
}

void SoundSettings::SetBgmLevel(int level) { bgmLevel_ = std::clamp(level, 0, kMaxLevel); }

void SoundSettings::SetSeLevel(int level) { seLevel_ = std::clamp(level, 0, kMaxLevel); }
