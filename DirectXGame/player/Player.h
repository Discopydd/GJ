#pragma once
#include "KamataEngine.h"
#include "../map/MapChipField.h"
using namespace KamataEngine;

/// <summary>
/// プレイヤークラス
/// </summary>
class Player {
public:
    Player() = default;
    ~Player();

    // 初期化（カメラ・マップ・モデル名指定）
    void Initialize(const Camera* camera, const MapChipField* map,const char* modelName = "player"); 
    // ワールド座標を直接設定
    void SetWorldPosition(const Vector3& pos);
    // タイル座標をワールド座標に変換して位置設定（Yは任意指定）
    void SetByTileIndex(const MapChipField& map, uint32_t xIndex, uint32_t yIndex, float yOnTop);

    // 現在タイル座標を取得
    uint32_t TileX() const { return ix_; }
    uint32_t TileY() const { return iy_; }

    // 毎フレーム更新
    void Update();  
    // 描画
    void Draw();
    // プレイヤーの高さを取得
    float GetHeight() const { return height_; }
    // 移動にかかる時間を取得
    float GetMoveDuration() const { return MoveDuration; }

private:
    // ===== 変換 / モデル / 依存関係 =====
    WorldTransform wt_;
    Model* model_ = nullptr;
    Input* input_ = nullptr;
    const Camera* camera_ = nullptr;
    const MapChipField* map_ = nullptr;

    // ===== タイル / 移動関連 =====
    float height_ = 1.5f;          // プレイヤーの高さ
    uint32_t ix_ = 0;              // 現在のタイルX
    uint32_t iy_ = 0;              // 現在のタイルY
    float MoveDuration = 0.18f;    // 1マス移動にかかる時間

    // 平滑移動の状態管理
    bool   isMoving_ = false;
    float  moveT_ = 0.0f;          // 補間 [0,1]
    Vector3 startPos_{};
    Vector3 targetPos_{};

    // ====== 回転（Y軸回り） ======
    // yawの定義：+X=0, +Z=+PI/2, -X=PI, -Z=-PI/2
    float currentYaw_ = 0.0f;
    float startYaw_   = 0.0f;
    float targetYaw_  = 0.0f;
    bool  isRotating_ = false;
    float rotateT_    = 0.0f;       // 補間 [0,1]
    float RotateDuration = 0.12f;   // 回転にかかる時間

    // 回転補助関数
    static float NormalizeAngle(float a);             // 角度を(-PI, PI]に正規化
    static float ShortestDelta(float from, float to); // 最短角度差を計算
    void RequestFaceYaw(float yaw);                   // 回転リクエスト
    void UpdateRotation(float dt);                    // 毎フレーム回転処理
};
