#pragma once
#include "math/Vector3.h"
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using namespace KamataEngine;
enum class MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
	kPortal,
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};
class MapChipField {

	MapChipData mapChipData_;

public:
	// １マスのサイズ
	static inline const float kBlockWidth = 2;
	static inline const float kBlockHeight = 2;
	// 縦横幅
	uint32_t numBlockVertical_  = 0;
	uint32_t numBlockHorizontal_  = 0;


	void ResetMapChipData();
	void LoadMapChipCsv(const std::string& filePath);
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex)const;
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex)const;
	struct IndexSet {
	uint32_t xIndex;
	uint32_t yIndex;
};
	IndexSet GetMapChipIndexByPosition(const Vector3& position)const;
	struct Rect {
		float left;
		float right;
		float bottom;
		float top;
	};
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex)const;

	Vector3 GetMapMinPosition() const {
		return Vector3(0, 0, 0);
	}
	Vector3 GetMapMaxPosition() const {
		return Vector3(
			numBlockHorizontal_  * kBlockWidth,
			numBlockVertical_  * kBlockHeight,
			0
		);
	}
};