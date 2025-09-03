#pragma once
#include "KamataEngine.h"
#include "../map/MapChipField.h"
using namespace KamataEngine;

class Player {
public:
    Player() = default;
    ~Player();

    void Initialize(const Camera* camera, const char* modelName = "player"); // 如果没有player模型，可传 "cube"
    void SetWorldPosition(const Vector3& pos);
    // 把地图索引转换为世界坐标并设置到 XZ，Y 给定（一般用方块高度）
    void SetByTileIndex(const MapChipField& map, uint32_t xIndex, uint32_t yIndex, float yOnTop);

    void Update();  // 预留（暂不做输入）
    void Draw();
    float GetHeight() const { return height_; }
private:
    WorldTransform wt_;
    Model* model_ = nullptr;
    const Camera* camera_ = nullptr;
    float height_ = 1.5f;
};
