#pragma once
#include "KamataEngine.h"
#include <array>
#include <memory>
#include <vector>

// 背景：夕暮れの空と、奥行きのある3層のビル群
// ビルはステージごとに生成し、カメラの動きに対して遠い層ほどゆっくり動く（視差）
class CityBackground {
public:
	~CityBackground();

	// stageWidth : ステージの横幅（ビルを並べる範囲を決める）
	// seed       : ビルの並びを決める乱数の種（同じ値なら毎回同じ街並みになる）
	void Initialize(KamataEngine::Camera* camera, float stageWidth, uint32_t seed);
	void Update();
	void Draw();

private:
	// ビルのモデルの種類（モデルの元の幅と高さ）
	struct BuildingModel {
		const char* name;
		float width;
		float height;
	};

	// 奥行きの層
	struct LayerInfo {
		float followRate;            // カメラに付いていく割合（1に近いほど遠くに見える）
		float z;                     // 奥行き
		KamataEngine::Vector4 tint;  // 色合い（遠いほど空の色に近く薄く）
		float minScale, maxScale;    // ビルの大きさ
		float minGap, maxGap;        // ビルどうしの間隔（マイナスで重なる）
	};

	struct Building {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor objectColor;
		KamataEngine::Model* model = nullptr;
		// カメラが原点にある時の位置
		KamataEngine::Vector3 basePosition = {};
		float followRate = 0.0f;
	};

	static inline const std::array<BuildingModel, 5> kBuildingModels = {{
	    {"building1", 3.0f, 10.0f},
	    {"building2", 4.0f, 7.0f},
	    {"building3", 2.0f, 14.0f},
	    {"building4", 5.0f, 4.0f},
	    {"building5", 3.0f, 16.0f},
	}};

	// 遠い層から順に描く
	static inline const std::array<LayerInfo, 3> kLayers = {{
	    {0.85f, 60.0f, {0.42f, 0.44f, 0.72f, 1.0f}, 0.75f, 1.15f, 0.0f, 2.0f},
	    {0.65f, 40.0f, {0.42f, 0.38f, 0.58f, 1.0f}, 0.55f, 0.85f, 1.0f, 3.5f},
	    {0.4f, 20.0f, {0.24f, 0.23f, 0.33f, 1.0f}, 0.35f, 0.55f, 2.0f, 5.0f},
	}};

	// 空の板（画面より少し大きく）
	static inline const float kSkyWidth = 30.0f;
	static inline const float kSkyHeight = 17.0f;
	static inline const float kSkyZ = 80.0f;
	// 普段のカメラの高さ（この高さの時にビルの足元が地面の下に隠れる）
	static inline const float kBaseCameraY = 6.0f;
	// ビルの足元の高さ（地面より下にして根元を隠す）
	static inline const float kBuildingBaseY = -1.0f;
	// 画面の横幅の半分（少し余裕を持たせる）
	static inline const float kViewHalfWidth = 13.0f;

	void UpdateTransform(KamataEngine::WorldTransform& worldTransform);

	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::Model* skyModel_ = nullptr;
	KamataEngine::WorldTransform skyTransform_;

	std::array<KamataEngine::Model*, kBuildingModels.size()> buildingModels_ = {};
	// WorldTransform はコピーできないので、ポインタで持つ
	std::vector<std::unique_ptr<Building>> buildings_;
};
