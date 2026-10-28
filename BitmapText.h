#pragma once
#include "KamataEngine.h"
#include <string>
#include <vector>

// font.png を使って文字列をスプライトで描画する（UTF-8 の日本語も描ける）
//   1〜4行目：英数字・記号（ASCII 32〜95。小文字は大文字で描く）。ドット文字で、送り幅は文字サイズの 0.8 倍
//   5行目から：日本語の文字（FontGlyphs.h に並んだ順）。送り幅は文字サイズと同じ
// 日本語の文字を新しく使ったら、Tools/genfont.py を実行して font.png と FontGlyphs.h を作り直す
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
	// UTF-8 の文字列を1文字ずつ（Unicode の番号に）分ける
	static std::u32string Decode(const std::string& text);
	// 1文字の送り幅（文字サイズに対する割合）
	static float AdvanceRate(char32_t c);
	// フォント画像の何番目のマスか（フォントにない文字と空白は -1）
	static int GlyphIndex(char32_t c);

	void PutChar(char32_t c, float x, float y, float charSize, const KamataEngine::Vector4& color);

	// フォント画像1文字分のピクセル数
	static inline const float kCellSize = 32.0f;
	static inline const uint32_t kColumns = 16;
	// 英数字のマスの数（ASCII 32〜95）
	static inline const int kAsciiGlyphCount = 64;
	// 1文字あたりの送り幅（文字サイズに対する割合）
	static inline const float kAdvanceRate = 0.8f;
	static inline const float kWideAdvanceRate = 1.0f;
	// 影のずらし幅（文字サイズに対する割合）
	static inline const float kShadowOffsetRate = 0.08f;

	uint32_t textureHandle_ = 0;
	std::vector<KamataEngine::Sprite*> sprites_;
	uint32_t usedCount_ = 0;
};
