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

    void SetLoadingTexture(const std::string& path) { loadingTexPath_ = path; needBuildLoading_ = true; }
    void ShowLoading(bool on) { showLoading_ = on; }
private:
    IScene* scene_ = nullptr;

    // ★ 改为“待切换”的场景（不立刻切）
    IScene* pendingScene_ = nullptr;

    // ★ 过渡状态机
    enum class Trans { Idle, FadeOut, Switch, FadeIn };
    Trans trans_ = Trans::Idle;

    // ★ 黑幕
    KamataEngine::Sprite* overlay_ = nullptr;
    float overlayAlpha_ = 0.0f;       // 0~1
    float overlaySpeed_ = 0.05f;      // 过渡速度（可调）

    KamataEngine::Sprite* loadingSprite_ = nullptr;
    std::string loadingTexPath_ = "Loading.png"; // 默认资源路径（可改）
    bool showLoading_ = false;                   // 是否显示 Loading（只在过渡中绘制）
    bool needBuildLoading_ = true;               // 延迟创建贴图

    void EnsureOverlay_();
    void StartFadeOut_();
    void DoSwitch_();
    void EnsureLoading_();       // 
    void DestroyLoading_();      //
};
