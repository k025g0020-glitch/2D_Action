#include "MapChipField.h"
#include <fstream>
#include <map>
#include <sstream>
#include <algorithm>
#include <cassert>

using namespace KamataEngine;

namespace {

std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
    {"2", MapChipType::kCoin},
    {"3", MapChipType::kPlayerStart},
    {"4", MapChipType::kGroundEnemy},
    {"5", MapChipType::kFlyingEnemy},
    {"6", MapChipType::kInvincibleEnemy},
    {"7", MapChipType::kGoal},
    {"8", MapChipType::kCheckpoint},
};
}

void MapChipField::Initialize() {}

void MapChipField::Update() {}

void MapChipField::Draw() {}

void MapChipField::ResetMapChipData() {
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirtical);
	for (std::vector<MapChipType>& mapChipDataLine : mapChipData_.data) {
		mapChipDataLine.resize(kNumBlockHorizontal);
	}
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
	ResetMapChipData();

	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	std::stringstream mapChipCsv;
	mapChipCsv << file.rdbuf();
	file.close();

	stageWidth_ = 0;

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		std::string line;
		getline(mapChipCsv, line);

		std::istringstream lineStream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {

			std::string word;
			std::getline(lineStream, word, ',');

			// 改行コード(CRLF)や空白を取り除く
			while (!word.empty() && (word.back() == '\r' || word.back() == ' ')) {
				word.pop_back();
			}

			if (word.empty()) {
				break;
			}

			if (mapChipTable.contains(word)) {
				mapChipData_.data[i][j] = mapChipTable[word];
			}

			stageWidth_ = (std::max)(stageWidth_, j + 1);
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex >= kNumBlockHorizontal || yIndex >= kNumBlockVirtical) {
		return MapChipType::kBlank;
	}
	return mapChipData_.data[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
	return Vector3(kBlockWidth * static_cast<float>(xIndex), kBlockHeight * (static_cast<float>(kNumBlockVirtical) - 1.0f - static_cast<float>(yIndex)), 0.0f);
}

MapChipField::IndexSet MapChipField::GetMapChipIndexByPosition(const Vector3& position) {
	IndexSet index = {};

	const float xIndexF = (position.x + kBlockWidth / 2.0f) / kBlockWidth;
	const float yIndexF = (position.y + kBlockHeight / 2.0f) / kBlockHeight;

	// マップの外（穴の下など）は範囲外のインデックスにして空白扱いにする
	if (xIndexF < 0.0f) {
		index.x = kNumBlockHorizontal;
	} else {
		index.x = static_cast<uint32_t>(xIndexF);
	}

	if (yIndexF < 0.0f) {
		index.y = kNumBlockVirtical;
	} else {
		uint32_t yIndexBeforeInverse = static_cast<uint32_t>(yIndexF);
		index.y = kNumBlockVirtical - 1 - yIndexBeforeInverse;
	}
	return index;
}

MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {
	// 指定ブロックの中心座標を取得する
	Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;
	rect.left = center.x - kBlockWidth / 2.0f;
	rect.right = center.x + kBlockWidth / 2.0f;
	rect.bottom = center.y - kBlockHeight / 2.0f;
	rect.top = center.y + kBlockHeight / 2.0f;

	return rect;
}