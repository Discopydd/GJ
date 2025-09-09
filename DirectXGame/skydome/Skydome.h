#pragma once
#include <KamataEngine.h>
/// <summary>
/// 天球
/// </summary>
class Skydome {
public:
    Skydome() = default;
    ~Skydome();

    // 初始化：传入场景用的相机；可选模型名与缩放
    void Initialize(KamataEngine::Camera* camera, const char* modelName = "Skydome");
    // 每帧：将天球中心对齐到相机位置
    void Update();
    // 绘制
    void Draw();

    // 可选：动态改缩放/相机
    void SetScale(float s) { worldTransform_.scale_ = { s, s, s }; }
    void SetCamera( KamataEngine::Camera* camera) { camera_ = camera; }
    void SetModel(const char* modelName);
private:
     KamataEngine::WorldTransform worldTransform_{};
     KamataEngine::Model* model_  = nullptr;
     KamataEngine::Camera* camera_ = nullptr;
};
