#include "CityBackground.h"
#include "calc.h"
#include <cassert>
#include <random>

using namespace KamataEngine;

CityBackground::~CityBackground() {
	buildings_.clear();
	delete skyModel_;
	for (Model* model : buildingModels_) {
		delete model;
	}
}

void CityBackground::Initialize(Camera* camera, float stageWidth, uint32_t seed) {
	assert(camera);
	camera_ = camera;

	skyModel_ = Model::CreateFromOBJ("citysky", true);
	skyTransform_.Initialize();
	skyTransform_.scale_ = {kSkyWidth, kSkyHeight, 1.0f};

	for (size_t i = 0; i < kBuildingModels.size(); ++i) {
		buildingModels_[i] = Model::CreateFromOBJ(kBuildingModels[i].name, true);
	}

	// 同じステージなら毎回同じ街並みになるように、種を固定した乱数を使う
	std::mt19937 engine(seed);
	auto random = [&engine](float min, float max) { return std::uniform_real_distribution<float>(min, max)(engine); };
	std::uniform_int_distribution<size_t> pickModel(0, kBuildingModels.size() - 1);

	for (const LayerInfo& layer : kLayers) {
		// カメラが x = halfWidth〜(stageWidth - halfWidth) を動く間に見える範囲だけ並べる
		// ビルの画面上の位置 = base + camera * rate なので、base は camera * (1 - rate) を中心に見えればよい
		const float minCameraX = kViewHalfWidth;
		const float maxCameraX = (std::max)(stageWidth - kViewHalfWidth, minCameraX);
		const float startX = minCameraX * (1.0f - layer.followRate) - kViewHalfWidth - 4.0f;
		const float endX = maxCameraX * (1.0f - layer.followRate) + kViewHalfWidth + 4.0f;

		float x = startX;
		while (x < endX) {
			const size_t modelIndex = pickModel(engine);
			const BuildingModel& info = kBuildingModels[modelIndex];
			const float scale = random(layer.minScale, layer.maxScale);
			const float width = info.width * scale;

			auto building = std::make_unique<Building>();
			building->model = buildingModels_[modelIndex];
			building->followRate = layer.followRate;
			building->basePosition = {x + width * 0.5f, kBuildingBaseY, layer.z};
			building->worldTransform.Initialize();
			building->worldTransform.scale_ = {scale, scale, 1.0f};

			// 同じ層の中でも少しずつ明るさを変える
			const float brightness = random(0.85f, 1.1f);
			building->objectColor.Initialize();
			building->objectColor.SetColor({layer.tint.x * brightness, layer.tint.y * brightness, layer.tint.z * brightness, 1.0f});

			buildings_.push_back(std::move(building));
			x += width + random(layer.minGap, layer.maxGap);
		}
	}

	Update();
}

void CityBackground::UpdateTransform(WorldTransform& worldTransform) {
	worldTransform.matWorld_ = calc::MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
	worldTransform.TransferMatrix();
}

void CityBackground::Update() {
	const Vector3& cameraPosition = camera_->translation_;
	// プレイヤーが高く登ってカメラが上がった分
	const float cameraRise = cameraPosition.y - kBaseCameraY;

	// 空はカメラにぴったり付いていく（画面に対して動かない）
	skyTransform_.translation_ = {cameraPosition.x, cameraPosition.y - kSkyHeight * 0.5f, kSkyZ};
	UpdateTransform(skyTransform_);

	for (const std::unique_ptr<Building>& building : buildings_) {
		WorldTransform& worldTransform = building->worldTransform;
		worldTransform.translation_ = {
		    building->basePosition.x + cameraPosition.x * building->followRate,
		    building->basePosition.y + cameraRise * building->followRate,
		    building->basePosition.z,
		};
		UpdateTransform(worldTransform);
	}
}

void CityBackground::Draw() {
	skyModel_->Draw(skyTransform_, *camera_);
	for (const std::unique_ptr<Building>& building : buildings_) {
		building->model->Draw(building->worldTransform, *camera_, &building->objectColor);
	}
}
