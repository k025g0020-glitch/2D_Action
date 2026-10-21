#include "BitmapText.h"
#include <cctype>

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

float BitmapText::MeasureWidth(const std::string& text, float charSize) {
	return charSize * kAdvanceRate * static_cast<float>(text.size());
}

void BitmapText::Print(const std::string& text, float x, float y, float charSize, const Vector4& color, Align align, bool shadow) {
	const float width = MeasureWidth(text, charSize);
	if (align == Align::kCenter) {
		x -= width * 0.5f;
	} else if (align == Align::kRight) {
		x -= width;
	}

	const float advance = charSize * kAdvanceRate;

	// 背景に溶け込まないように、先に黒い影を描く
	if (shadow) {
		const float offset = charSize * kShadowOffsetRate;
		const Vector4 shadowColor = {0.0f, 0.0f, 0.0f, color.w * 0.8f};
		for (size_t i = 0; i < text.size(); ++i) {
			PutChar(text[i], x + advance * static_cast<float>(i) + offset, y + offset, charSize, shadowColor);
		}
	}

	for (size_t i = 0; i < text.size(); ++i) {
		PutChar(text[i], x + advance * static_cast<float>(i), y, charSize, color);
	}
}

void BitmapText::PutChar(char c, float x, float y, float charSize, const Vector4& color) {
	// 小文字は大文字として扱う
	c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

	// フォントにない文字と空白は描画しない
	if (c <= ' ' || c > '_') {
		return;
	}
	if (usedCount_ >= sprites_.size()) {
		return;
	}

	const uint32_t index = static_cast<uint32_t>(c - ' ');
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
