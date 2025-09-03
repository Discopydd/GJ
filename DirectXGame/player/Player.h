#pragma once
#include "KamataEngine.h"
#include "../map/MapChipField.h"
using namespace KamataEngine;

class Player {
public:
    Player() = default;
    ~Player();

    void Initialize(const Camera* camera, const MapChipField* map,const char* modelName = "player"); // 如果没有player模型，可传 "cube"
    void SetWorldPosition(const Vector3& pos);
    // 把地图索引转换为世界坐标并设置到 XZ，Y 给定（一般用方块高度）
    void SetByTileIndex(const MapChipField& map, uint32_t xIndex, uint32_t yIndex, float yOnTop);

    // 读取当前网格索引
    uint32_t TileX() const { return ix_; }
    uint32_t TileY() const { return iy_; }

    void Update();  // 预留（暂不做输入）
    void Draw();
    float GetHeight() const { return height_; }
    float GetMoveDuration() const { return MoveDuration; }
private:
    WorldTransform wt_;
    Model* model_ = nullptr;
    Input* input_ = nullptr;
    const Camera* camera_ = nullptr;
    const MapChipField* map_ = nullptr;
    float height_ = 1.5f;
    uint32_t ix_ = 0;            // 当前所在格（列）
    uint32_t iy_ = 0;            // 当前所在格（行）
    float MoveDuration = 0.18f;

     // ★ 平滑移动状态
    bool   isMoving_ = false;
    float  moveT_ = 0.0f;          // [0,1]
    Vector3 startPos_{};
    Vector3 targetPos_{};
};
