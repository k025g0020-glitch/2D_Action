#include "SoundEffects.h"

using namespace KamataEngine;

void SoundEffects::Load() {
	if (isLoaded_) {
		return;
	}
	Audio* audio = Audio::GetInstance();
	for (size_t i = 0; i < handles_.size(); ++i) {
		handles_[i] = audio->LoadWave(kEntries[i].fileName);
	}
	isLoaded_ = true;
}

void SoundEffects::Play(Id id) {
	if (!isLoaded_) {
		return;
	}
	const size_t index = static_cast<size_t>(id);
	Audio::GetInstance()->PlayWave(handles_[index], false, kEntries[index].volume);
}
