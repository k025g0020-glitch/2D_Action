#include "VolumeMenu.h"
#include "BgmPlayer.h"
#include "MenuInput.h"
#include "SoundEffects.h"
#include "SoundSettings.h"
#include <cmath>
#include <cstdio>
#include <string>

using namespace KamataEngine;

namespace {
const Vector4 kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
const Vector4 kYellow = {1.0f, 0.9f, 0.3f, 1.0f};
const Vector4 kGray = {0.6f, 0.62f, 0.7f, 1.0f};

// 「< =======--- >  70」のような音量の表示（左右の矢印で A / D で変えられることを示す）
std::string VolumeBar(int level) {
	std::string bar = "< ";
	for (int i = 0; i < SoundSettings::kMaxLevel; ++i) {
		bar += (i < level) ? '=' : '-';
	}
	char percent[8];
	std::snprintf(percent, sizeof(percent), "%4d", level * 100 / SoundSettings::kMaxLevel);
	return bar + " >" + percent;
}
} // namespace

void VolumeMenu::Open(bool selectClose) { item_ = selectClose ? Item::kClose : Item::kBgm; }

bool VolumeMenu::Update() {
	const Item previous = item_;
	item_ = MenuInput::SelectItem(item_);
	if (item_ != previous) {
		SoundEffects::Play(SoundEffects::Id::kCoin);
	}

	// A / D で音量を1段ずつ変える
	const int direction = MenuInput::Horizontal();
	if (direction != 0) {
		if (item_ == Item::kBgm) {
			SoundSettings::SetBgmLevel(SoundSettings::GetBgmLevel() + direction);
			// 流れている曲の音量をすぐ変えて、大きさを確かめられるようにする
			BgmPlayer::RefreshVolume();
		} else if (item_ == Item::kSe) {
			SoundSettings::SetSeLevel(SoundSettings::GetSeLevel() + direction);
			// 効果音を鳴らして大きさを確かめられるようにする
			SoundEffects::Play(SoundEffects::Id::kCoin);
		}
	}

	if (item_ == Item::kClose && MenuInput::Decide()) {
		SoundSettings::Save();
		SoundEffects::Play(SoundEffects::Id::kDecide);
		return true;
	}
	return false;
}

void VolumeMenu::Draw(BitmapText& text, const char* title, const char* closeLabel, float top, float blinkTimer) const {
	text.Print(title, 640.0f, top, 44.0f, kWhite, BitmapText::Align::kCenter);

	const struct {
		const char* label;
		int level;
	} kRows[] = {
	    {"BGM", SoundSettings::GetBgmLevel()},
	    {"SE ", SoundSettings::GetSeLevel()},
	};
	constexpr float kRowX = 280.0f;
	for (int i = 0; i < 2; ++i) {
		const bool selected = static_cast<int>(item_) == i;
		const std::string row = std::string(selected ? "> " : "  ") + kRows[i].label + "  " + VolumeBar(kRows[i].level);
		text.Print(row, kRowX, top + 80.0f + 56.0f * i, 36.0f, selected ? kYellow : kWhite);
	}

	// 閉じる項目は、選んでいる時に矢印を点滅させる
	const bool closeSelected = item_ == Item::kClose;
	std::string close = closeLabel;
	if (closeSelected && std::fmod(blinkTimer, 1.0f) < 0.6f) {
		close = "> " + close + " <";
	}
	text.Print(close, 640.0f, top + 210.0f, 36.0f, closeSelected ? kYellow : kWhite, BitmapText::Align::kCenter);

	std::string hint = std::string("W / S : SELECT   A / D : VOLUME   SPACE : ") + closeLabel;
	text.Print(hint, 640.0f, top + 290.0f, 24.0f, kGray, BitmapText::Align::kCenter);
}
