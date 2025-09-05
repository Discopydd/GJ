#include "MapChipField.h"
using namespace KamataEngine;
namespace {

std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType ::kBlank},
    {"1", MapChipType::kBlock },
    {"2", MapChipType::kPortal },
    {"3", MapChipType::kRaised },
    {"4", MapChipType::kSpike }
};

}

void MapChipField::ResetMapChipData() {
	mapChipData_.data.clear();
	numBlockVertical_ = 0;
    numBlockHorizontal_ = 0;
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
    ResetMapChipData();
   std::ifstream file(filePath);
    assert(file.is_open());

    std::string line;
    std::vector<std::vector<MapChipType>> tempData;

       while (std::getline(file, line)) {
        if (line.empty()) continue; // skip
        std::istringstream line_stream(line);
        std::vector<MapChipType> row;
        std::string word;
        while (std::getline(line_stream, word, ',')) {
            if (mapChipTable.contains(word)) {
                row.push_back(mapChipTable[word]);
            } else {
                row.push_back(MapChipType::kBlank);
            }
        }
        if (!row.empty()) {
            tempData.push_back(row);
        }
    }
    file.close();

    if (tempData.empty()) {
        numBlockVertical_ = 0;
        numBlockHorizontal_ = 0;
        return; // 空地图
    }
    numBlockHorizontal_ = 0;
    for (auto& row : tempData) {
        if (row.size() > numBlockHorizontal_) {
            numBlockHorizontal_ = static_cast<uint32_t>(row.size());
        }
    }
    numBlockVertical_ = static_cast<uint32_t>(tempData.size());

    mapChipData_.data.resize(numBlockVertical_);
    for (uint32_t i = 0; i < numBlockVertical_; i++) {
        uint32_t reversedIndex = numBlockVertical_ - 1 - i;
        mapChipData_.data[reversedIndex].resize(numBlockHorizontal_, MapChipType::kBlank);
        for (uint32_t j = 0; j < tempData[i].size(); j++) {
            mapChipData_.data[reversedIndex][j] = tempData[i][j];
        }
    }
}
MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex)const {
	if (xIndex >= numBlockHorizontal_  || yIndex >= numBlockVertical_) {
        return MapChipType::kBlank;
    }
    return mapChipData_.data[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) const{  return Vector3(
        kBlockWidth * xIndex + kBlockWidth / 2.0f, // X 位置
        kBlockHeight * yIndex + kBlockHeight / 2.0f, // Y 位置
        0.0f
    ); }

MapChipField::IndexSet MapChipField::GetMapChipIndexByPosition(const Vector3& position)const
{
   IndexSet indexSet{};
    indexSet.xIndex = static_cast<uint32_t>(position.x / kBlockWidth);
    indexSet.yIndex = static_cast<uint32_t>(position.y / kBlockHeight);
    return indexSet;
}

MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex)const
{
    Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);
    Rect rect{};
    rect.left = center.x - kBlockWidth / 2;
    rect.right = center.x + kBlockWidth / 2;
    rect.bottom = center.y - kBlockHeight / 2;
    rect.top = center.y + kBlockHeight / 2;

    return rect;
}

void MapChipField::SetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex, MapChipType type)
{
    if (yIndex >= mapChipData_.data.size()) return;
    if (xIndex >= mapChipData_.data[yIndex].size()) return;
    mapChipData_.data[yIndex][xIndex] = type;
}
