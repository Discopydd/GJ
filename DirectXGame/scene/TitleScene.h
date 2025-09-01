#pragma once
#include "KamataEngine.h"

class TitleScene {
public:
    ~TitleScene();
    void Initialize();
    void Update();
    void Draw();
    bool IsSceneEnd();  // Enterが押されたらtrue

private:
    uint32_t titleTextureHandle_ = 0;
    KamataEngine::Sprite* titleSprite_ = nullptr;
    uint32_t startTextureHandle_ = 0;
    KamataEngine::Sprite* startSprite_ = nullptr;
    KamataEngine::Input* input_ = nullptr;
    KamataEngine::DirectXCommon* dxCommon_ = nullptr;
    bool isSceneEnd_ = false;
    int frameCount_ = 0;
};
