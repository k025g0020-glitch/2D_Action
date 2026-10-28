#include "BitmapText.h"
#include "FontGlyphs.h"
#include <algorithm>
#include <iterator>

using namespace KamataEngine;

BitmapText::~BitmapText() {
	for (Sprite* sprite : sprites_) {
		delete sprite;
	}
	sprites_.clear();
}

void BitmapText::Initialize(uint32_t maxSprites) {
	textureHandle_ = TextureManager::Load("font/font.png");

	// スプライトは描画のたびに使い回すので、先にまとめて作っておく
	sprites_.reserve(maxSprites);
	for (uint32_t i = 0; i < maxSprites; ++i) {
		sprites_.push_back(Sprite::Create(textureHandle_, {0.0f, 0.0f}));
	}
	usedCount_ = 0;
}

void BitmapText::Clear() { usedCount_ = 0; }

std::u32string BitmapText::Decode(const std::string& text) {
	std::u32string result;
	for (size_t i = 0; i < text.size();) {
		const unsigned char c = static_cast<unsigned char>(text[i]);
		// 先頭のバイトで、何バイトの文字かが分かる
		int length = 1;
		char32_t code = c;
		if (c >= 0xF0) {
			length = 4;
			code = c & 0x07;
		} else if (c >= 0xE0) {
			length = 3;
			code = c & 0x0F;
		} else if (c >= 0xC0) {
			length = 2;
			code = c & 0x1F;
		}
		for (int k = 1; k < length && i + k < text.size(); ++k) {
			code = (code << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
		}
		result.push_back(code);
		i += length;
	}
	return result;
}

float BitmapText::AdvanceRate(char32_t c) { return (c < 0x80) ? kAdvanceRate : kWideAdvanceRate; }

int BitmapText::GlyphIndex(char32_t c) {
	// 英数字（小文字は大文字として扱う）
	if (c < 0x80) {
		if (c >= 'a' && c <= 'z') {
			c -= 'a' - 'A';
		}
		if (c <= U' ' || c > U'_') {
			return -1;
		}
		return static_cast<int>(c - U' ');
	}
	// 日本語：FontGlyphs.h の並び（小さい順）から探す
	const char32_t* begin = std::begin(kFontJapaneseGlyphs);
	const char32_t* end = std::end(kFontJapaneseGlyphs);
	const char32_t* found = std::lower_bound(begin, end, c);
	if (found == end || *found != c) {
		return -1;
	}
	return kAsciiGlyphCount + static_cast<int>(found - begin);
}

float BitmapText::MeasureWidth(const std::string& text, float charSize) {
	float width = 0.0f;
	for (char32_t c : Decode(text)) {
		width += charSize * AdvanceRate(c);
	}
	return width;
}

void BitmapText::Print(const std::string& text, float x, float y, float charSize, const Vector4& color, Align align, bool shadow) {
	const float width = MeasureWidth(text, charSize);
	if (align == Align::kCenter) {
		x -= width * 0.5f;
	} else if (align == Align::kRight) {
		x -= width;
	}

	const std::u32string chars = Decode(text);

	// 背景に溶け込まないように、先に黒い影を描く
	if (shadow) {
		const float offset = charSize * kShadowOffsetRate;
		const Vector4 shadowColor = {0.0f, 0.0f, 0.0f, color.w * 0.8f};
		float cursor = x;
		for (char32_t c : chars) {
			PutChar(c, cursor + offset, y + offset, charSize, shadowColor);
			cursor += charSize * AdvanceRate(c);
		}
	}

	float cursor = x;
	for (char32_t c : chars) {
		PutChar(c, cursor, y, charSize, color);
		cursor += charSize * AdvanceRate(c);
	}
}

void BitmapText::PutChar(char32_t c, float x, float y, float charSize, const Vector4& color) {
	// フォントにない文字と空白は描画しない
	const int index = GlyphIndex(c);
	if (index < 0) {
		return;
	}
	if (usedCount_ >= sprites_.size()) {
		return;
	}

	const float u = static_cast<float>(index % kColumns) * kCellSize;
	const float v = static_cast<float>(index / kColumns) * kCellSize;

	Sprite* sprite = sprites_[usedCount_++];
	sprite->SetTextureRect({u, v}, {kCellSize, kCellSize});
	sprite->SetSize({charSize, charSize});
	sprite->SetPosition({x, y});
	sprite->SetColor(color);
}

void BitmapText::Draw() {
	if (usedCount_ == 0) {
		return;
	}

	Sprite::PreDraw();
	for (uint32_t i = 0; i < usedCount_; ++i) {
		sprites_[i]->Draw();
	}
	Sprite::PostDraw();
}
