#pragma once
#include <random>

// エフェクト用の乱数（min〜max の一様乱数）
inline float RandomRange(float min, float max) {
	static std::mt19937 engine(std::random_device{}());
	std::uniform_real_distribution<float> distribution(min, max);
	return distribution(engine);
}
