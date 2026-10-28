#pragma once
#include "KamataEngine.h"

// メニューの操作（タイトルのメニュー・音量の設定・一時停止で共通）
namespace MenuInput {

// 項目を上下に選ぶ（W / S、矢印キーの上下）。端まで行ったら反対側に回る
// T は項目の数を kCount に持つ enum class
template <typename T> T SelectItem(T current) {
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();
	const int count = static_cast<int>(T::kCount);
	int index = static_cast<int>(current);
	if (input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP)) {
		index = (index + count - 1) % count;
	}
	if (input->TriggerKey(DIK_S) || input->TriggerKey(DIK_DOWN)) {
		index = (index + 1) % count;
	}
	return static_cast<T>(index);
}

// 左右の入力（A / D、矢印キーの左右）。-1 / 0 / +1
inline int Horizontal() {
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();
	int direction = 0;
	if (input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT)) {
		--direction;
	}
	if (input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT)) {
		++direction;
	}
	return direction;
}

// 決定（SPACE）
inline bool Decide() { return KamataEngine::Input::GetInstance()->TriggerKey(DIK_SPACE); }

} // namespace MenuInput
