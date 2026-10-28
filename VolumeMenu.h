#pragma once
#include "BitmapText.h"

// BGM と効果音の音量を変える画面（タイトルの SOUND と、ゲーム中の一時停止で使う）
//   BGM  < =======--- >  70
//   SE   < ========== > 100
//   BACK（一時停止では RESUME）
class VolumeMenu {
public:
	// 開いた時に呼ぶ。selectClose が true なら一番下の項目（RESUME など）を、false なら BGM の行を選んだ状態にする
	void Open(bool selectClose = false);
	// 一番下の項目（BACK / RESUME）を選んで決定したら true。その時に音量を保存する
	bool Update();
	// title : 上に出す見出し、closeLabel : 一番下の項目の名前、top : 見出しの y 座標
	void Draw(BitmapText& text, const char* title, const char* closeLabel, float top, float blinkTimer) const;

private:
	enum class Item {
		kBgm,   // BGM の音量
		kSe,    // 効果音の音量
		kClose, // 閉じる（BACK / RESUME）
		kCount,
	};

	Item item_ = Item::kBgm;
};
