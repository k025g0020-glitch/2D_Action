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

void VolumeMenu::Initialize(const std::vector<Row>& rows) {
	rows_ = rows;
	selected_ = 0;
}

void VolumeMenu::Open(int selectedRow) { selected_ = selectedRow; }

int VolumeMenu::Update() {
	Input* input = Input::GetInstance();
	const int count = static_cast<int>(rows_.size());

	// W / S で上下に選ぶ（端まで行ったら反対側に回る）
	const int previous = selected_;
	if (input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP)) {
		selected_ = (selected_ + count - 1) % count;
	}
	if (input->TriggerKey(DIK_S) || input->TriggerKey(DIK_DOWN)) {
		selected_ = (selected_ + 1) % count;
	}
	if (selected_ != previous) {
		SoundEffects::Play(SoundEffects::Id::kCoin);
	}

	const Row& row = rows_[selected_];
	// A / D で音量を1段ずつ変える
	const int direction = MenuInput::Horizontal();
	if (direction != 0 && row.type == RowType::kBgm) {
		SoundSettings::SetBgmLevel(SoundSettings::GetBgmLevel() + direction);
		// 流れている曲の音量をすぐ変えて、大きさを確かめられるようにする
		BgmPlayer::RefreshVolume();
	} else if (direction != 0 && row.type == RowType::kSe) {
		SoundSettings::SetSeLevel(SoundSettings::GetSeLevel() + direction);
		// 効果音を鳴らして大きさを確かめられるようにする
		SoundEffects::Play(SoundEffects::Id::kCoin);
	}

	if (row.type == RowType::kAction && MenuInput::Decide()) {
		SoundSettings::Save();
		SoundEffects::Play(SoundEffects::Id::kDecide);
		return selected_;
	}
	return -1;
}

void VolumeMenu::Draw(BitmapText& text, const char* title, float top, float blinkTimer, bool showHint) const {
	text.Print(title, 640.0f, top, 44.0f, kWhite, BitmapText::Align::kCenter);

	constexpr float kCharSize = 36.0f;
	// 音量の行：左端の位置と、そこから項目名・バーまでの距離
	constexpr float kVolumeRowX = 270.0f;
	constexpr float kVolumeLabelOffset = 50.0f;
	constexpr float kVolumeBarOffset = 200.0f;
	bool hasVolume = false;
	for (size_t i = 0; i < rows_.size(); ++i) {
		const Row& row = rows_[i];
		const bool selected = static_cast<int>(i) == selected_;
		const float y = top + 80.0f + kRowSpacing * static_cast<float>(i);
		const Vector4& color = selected ? kYellow : kWhite;

		if (row.type == RowType::kAction) {
			// 決定する項目は、選んでいる時に矢印を点滅させる
			std::string label = row.label;
			if (selected && std::fmod(blinkTimer, 1.0f) < 0.6f) {
				label = "> " + label + " <";
			}
			text.Print(label, 640.0f, y, kCharSize, color, BitmapText::Align::kCenter);
		} else {
			hasVolume = true;
			// 「>」・項目名・バーをそれぞれ決まった位置に描いて、項目名の長さが違ってもバーの位置をそろえる
			const int level = (row.type == RowType::kBgm) ? SoundSettings::GetBgmLevel() : SoundSettings::GetSeLevel();
			if (selected) {
				text.Print(">", kVolumeRowX, y, kCharSize, color);
			}
			text.Print(row.label, kVolumeRowX + kVolumeLabelOffset, y, kCharSize, color);
			text.Print(VolumeBar(level), kVolumeRowX + kVolumeBarOffset, y, kCharSize, color);
		}
	}

	if (!showHint) {
		return;
	}
	const float hintY = top + 80.0f + kRowSpacing * static_cast<float>(rows_.size()) + 24.0f;
	text.Print(hasVolume ? "W / S：選ぶ　　A / D：音量　　SPACE：決定" : "W / S：選ぶ　　SPACE：決定", 640.0f, hintY, 24.0f, kGray, BitmapText::Align::kCenter);
}
