#pragma once
#include "KamataEngine.h"
#include <string>
#include <vector>

// font.png（ASCII 32〜95 を16列×4行に並べた画像）を使って文字列をスプライトで描画する
// 毎フレーム Clear() → Print() → Draw() の順に呼ぶ
class BitmapText {
public:
	enum class Align {
		kLeft,
		kCenter,
		kRight,
	};

	~BitmapText();

	void Initialize(uint32_t maxSprites = 512);

	// 前のフレームに登録した文字列を消す
	void Clear();

	// 文字列を登録する（x, y は左上の座標、charSize は1文字の大きさ）
	void Print(const std::string& text, float x, float y, float charSize, const KamataEngine::Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f}, Align align = Align::kLeft, bool shadow = true);

	// 登録した文字列をまとめて描画する
	void Draw();

	// 文字列の幅を計算する
	static float MeasureWidth(const std::string& text, float charSize);

private:
	void PutChar(char c, float x, float y, float charSize, const KamataEngine::Vector4& color);

	// フォント画像1文字分のピクセル数
	static inline const float kCellSize = 32.0f;
	static inline const uint32_t kColumns = 16;
	// 1文字あたりの送り幅（文字サイズに対する割合）
	static inline const float kAdvanceRate = 0.8f;
	// 影のずらし幅（文字サイズに対する割合）
	static inline const float kShadowOffsetRate = 0.08f;

	uint32_t textureHandle_ = 0;
	std::vector<KamataEngine::Sprite*> sprites_;
	uint32_t usedCount_ = 0;
};
