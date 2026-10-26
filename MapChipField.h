#pragma once
#include "KamataEngine.h"


// CSVの数値とマップチップの対応
// 0:空白 1:ブロック 2:コイン 3:プレイヤー開始位置 4:地上敵 5:飛行敵 6:無敵敵 7:ゴールの星 8:中間ポイント
enum class MapChipType {
	kBlank,
	kBlock,
	kCoin,
	kPlayerStart,
	kGroundEnemy,
	kFlyingEnemy,
	kInvincibleEnemy,
	kGoal,
	kCheckpoint,
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField {
public:

	// 範囲矩形構造体を追加
	struct Rect {
		float left;   // 左端
		float right;  // 右端
		float bottom; // 下端
		float top;    // 上端
	};

	void Initialize();
	void Update();
	void Draw();
	void ResetMapChipData();
	void LoadMapChipCsv(const std::string& filePath);

	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	KamataEngine::Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

	struct IndexSet {
		uint32_t x;
		uint32_t y;
	};

	IndexSet GetMapChipIndexByPosition(const KamataEngine::Vector3& position);

	uint32_t GetNumBlockVirtical() const {
		return kNumBlockVirtical;
	}
	uint32_t GetNumBlockHorizontal() const { 
		return kNumBlockHorizontal; 
	}
	// CSVに実際に書かれていた横方向のブロック数（ステージの幅）
	uint32_t GetStageWidth() const { return stageWidth_; }
	float GetBlockWidth() const { return kBlockWidth; }
	float GetBlockHeight() const { return kBlockHeight; }


private:

	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;

	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 300;

	MapChipData mapChipData_;

	uint32_t stageWidth_ = 0;

};