#pragma once
#include "BitmapText.h"
#include <vector>

// 項目を縦に並べたメニュー。BGM と効果音の音量の行を入れられる
// （タイトルの SOUND と、ゲーム中のポーズで使う）
//     RESUME
//     RETRY
//   > BGM  < =======--- >  70
//     SE   < ========== > 100
class VolumeMenu {
public:
	enum class RowType {
		kBgm,    // BGM の音量（A / D で変える）
		kSe,     // 効果音の音量（A / D で変える）
		kAction, // 決定する項目（BACK / RESUME / RETRY / QUIT など）
	};
	struct Row {
		RowType type;
		const char* label;
	};

	void Initialize(const std::vector<Row>& rows);
	// 開いた時に呼ぶ。selectedRow の行を選んだ状態にする
	void Open(int selectedRow = 0);
	// 決定する項目を選んで SPACE を押したら、その行の番号を返す（それ以外は -1）。決定した時に音量を保存する
	int Update();
	// title : 上に出す見出し、top : 見出しの y 座標、showHint : 下に操作説明を出すか
	void Draw(BitmapText& text, const char* title, float top, float blinkTimer, bool showHint = true) const;

	// 行と行の間隔
	static inline const float kRowSpacing = 52.0f;

private:
	std::vector<Row> rows_;
	int selected_ = 0;
};
