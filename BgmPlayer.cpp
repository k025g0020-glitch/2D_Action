#include "BgmPlayer.h"
#include "SoundSettings.h"
#include <algorithm>

using namespace KamataEngine;

void BgmPlayer::Load() {
	if (isLoaded_) {
		return;
	}
	Audio* audio = Audio::GetInstance();
	for (size_t i = 0; i < handles_.size(); ++i) {
		handles_[i] = audio->LoadWave(kEntries[i].fileName);
	}
	isLoaded_ = true;
}

void BgmPlayer::Play(Id id) {
	if (!isLoaded_ || id == current_) {
		return;
	}
	Stop();
	const size_t index = static_cast<size_t>(id);
	voiceHandle_ = Audio::GetInstance()->PlayWave(handles_[index], true, kEntries[index].volume * SoundSettings::GetBgmScale());
	current_ = id;
}

void BgmPlayer::RefreshVolume() {
	if (current_ == Id::kNone) {
		return;
	}
	const size_t index = static_cast<size_t>(current_);
	Audio::GetInstance()->SetVolume(voiceHandle_, kEntries[index].volume * SoundSettings::GetBgmScale());
}

void BgmPlayer::Stop() {
	if (current_ == Id::kNone) {
		return;
	}
	Audio::GetInstance()->StopWave(voiceHandle_);
	current_ = Id::kNone;
}

BgmPlayer::Id BgmPlayer::StageBgm(uint32_t stageIndex) {
	static const Id kStageBgms[] = {Id::kStage1, Id::kStage2, Id::kStage3};
	return kStageBgms[std::min<size_t>(stageIndex, std::size(kStageBgms) - 1)];
}
