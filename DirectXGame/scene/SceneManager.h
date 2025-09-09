#pragma once
#include "IScene.h"

#include "KamataEngine.h"

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    // 不改调用方式：仍用 SetNextScene 预约切场景
    void SetNextScene(IScene* scene, bool useTransition = true);
    void Update();
    void Draw();
    void SetInitialScene(IScene* scene) { SetNextScene(scene, /*useTransition=*/false); }
    void ShowLoadingOnNextSwitch(bool enable = true) { showLoadingThisSwitch_ = enable; }
private:
    IScene* scene_ = nullptr;

    // ★ 改为“待切换”的场景（不立刻切）
    IScene* pendingScene_ = nullptr;

    // ★ 过渡状态机
    enum class Trans { Idle, FadeOut, Switch, FadeIn };
    Trans trans_ = Trans::Idle;

    // ★ 黑幕
    KamataEngine::Sprite* overlay_ = nullptr;
    KamataEngine::Sprite* loadingSprite_ = nullptr;
    float overlayAlpha_ = 0.0f;       // 0~1
    float overlaySpeed_ = 0.05f;      // 过渡速度（可调）
    bool  switchStarted_ = false; 
    bool showLoadingThisSwitch_ = false; 
    void EnsureOverlay_();            // 创建黑幕
    void StartFadeOut_();             // 进入淡出
    void DoSwitch_();                 // 真正删除旧场景并初始化新场景
    void EnsureLoading_();
};
