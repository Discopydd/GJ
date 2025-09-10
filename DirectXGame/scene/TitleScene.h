#pragma once
#include "KamataEngine.h"
// 场景管理 & 目标场景
#include "SceneManager.h"
#include "LevelSelectScene.h"
#include "IScene.h"
class TitleScene  : public IScene {
public:
    ~TitleScene()override;
    void Initialize()override;
    void Update()override;
    void Draw()override;
    void Finalize() override;  
    bool IsSceneEnd() const;
    void SetSceneManager(SceneManager* sm) { sceneManager_ = sm; }
private:
    uint32_t titleTextureHandle_ = 0;
    KamataEngine::Sprite* titleSprite_ = nullptr;
    uint32_t startTextureHandle_ = 0;
    KamataEngine::Sprite* startSprite_ = nullptr;
    uint32_t backTextureHandle_ = 0;
    KamataEngine::Sprite* backSprite_ = nullptr;
    KamataEngine::Input* input_ = nullptr;
    KamataEngine::DirectXCommon* dxCommon_ = nullptr;
    bool isSceneEnd_ = false;
    int frameCount_ = 0;

    SceneManager* sceneManager_ = nullptr;
    uint32_t seClick_ = 0;

      // ==== BGM ====
    uint32_t bgmHandle_ = 0;   // BGM 音源
    uint32_t bgmVoice_ = 0;    // 播放中的句柄
    float bgmVolume_ = 1.0f;   // 当前音量
    bool fadingOut_ = false;   // 是否在淡出
};
