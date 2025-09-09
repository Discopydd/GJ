#include "SceneManager.h"
#include "LevelSelectScene.h"
#include "GameScene.h"
using namespace KamataEngine;

SceneManager::SceneManager() {
    EnsureOverlay_();
    //EnsureLoading_();
}

SceneManager::~SceneManager() {
    if (scene_) { scene_->Finalize(); delete scene_; scene_ = nullptr; }
    if (pendingScene_) { delete pendingScene_; pendingScene_ = nullptr; }
    delete overlay_; overlay_ = nullptr;
    delete loadingSprite_; loadingSprite_ = nullptr;
}

void SceneManager::EnsureOverlay_() {
    if (overlay_) return;
    uint32_t white = TextureManager::Load("white1x1.png");   // Resources/white1x1.png
    overlay_ = Sprite::Create(white, {0,0});
    overlay_->SetAnchorPoint({0,0});
    overlay_->SetSize({ (float)WinApp::kWindowWidth, (float)WinApp::kWindowHeight });
    overlay_->SetColor({0,0,0,overlayAlpha_});
}

void SceneManager::SetNextScene(IScene* scene, bool useTransition) {
    if (pendingScene_) { delete pendingScene_; }
    pendingScene_ = scene;

    // ★ 第一次或明确要求不使用过渡：立即切换
    if (!useTransition || scene_ == nullptr) {
        // 删除旧场景（首次时 scene_==nullptr，不会进）
        if (scene_) { scene_->Finalize(); delete scene_; scene_ = nullptr; }

        scene_ = pendingScene_;
        pendingScene_ = nullptr;

        EnsureOverlay_();                 // 确保黑幕已创建
        overlayAlpha_ = 0.0f;             // 不显示黑幕
        if (overlay_) overlay_->SetColor({ 0,0,0,0 });
        trans_ = Trans::Idle;             // 不进入 Fade 状态机

        if (scene_) { scene_->Initialize(); }  // 直接初始化并显示
        return;
    }

    // ★ 正常路径：使用过渡
    if (trans_ == Trans::Idle) {
        StartFadeOut_();
    }
}

void SceneManager::StartFadeOut_() {
    EnsureOverlay_();
    if (showLoadingThisSwitch_) {
        EnsureLoading_();          // 只在需要时才加载/创建 Loading 精灵
        switchStarted_ = false;    // Switch 第一帧先显示 Loading
    }
    trans_ = Trans::FadeOut;
}

void SceneManager::DoSwitch_() {
    // 删除旧场景，初始化新场景（此时屏幕已黑）
    if (scene_) { scene_->Finalize(); delete scene_; scene_ = nullptr; }
    scene_ = pendingScene_;
    pendingScene_ = nullptr;
    if (scene_) { scene_->Initialize(); }    // 即便耗时，上一帧已是全黑
    trans_ = Trans::FadeIn;                  // 切完准备淡入
}

void SceneManager::EnsureLoading_()
{
    // 准备一张 Resources/ui/loading.png
    uint32_t tex = KamataEngine::TextureManager::Load("loading.png");
    loadingSprite_ = KamataEngine::Sprite::Create(tex, { 0,0 });
}

void SceneManager::Update() {
    switch (trans_) {
    case Trans::FadeOut:
        overlayAlpha_ += overlaySpeed_;
        if (overlayAlpha_ >= 1.0f) {
            overlayAlpha_ = 1.0f;
            trans_ = Trans::Switch;
            switchStarted_ = false; // 下一帧先画 Loading（静态）
        }
        break;

    case Trans::Switch:
        if (!switchStarted_) {
            // 第一次进入 Switch：仅渲染“黑幕 + Loading”这一帧
            switchStarted_ = true;
            // 保持 loading 居中（若窗口大小可能变化，逐帧更新位置）
            if (loadingSprite_) {
                loadingSprite_->SetPosition({
                  (float)KamataEngine::WinApp::kWindowWidth * 0.5f,
                  (float)KamataEngine::WinApp::kWindowHeight * 0.7f   // 靠下些更像加载提示
                    });
            }
            // 本帧不做 Initialize，让这帧画面能先显示出去
        }
        else {
            DoSwitch_();  // 下一帧再真正 Initialize 新场景
        }
        break;

    case Trans::FadeIn:
        overlayAlpha_ -= overlaySpeed_;
        if (overlayAlpha_ <= 0.0f) { overlayAlpha_ = 0.0f; trans_ = Trans::Idle; }
        break;

    case Trans::Idle:
        break;
    }

    if (overlay_) overlay_->SetColor({ 0,0,0,overlayAlpha_ });

    // 非 Switch 阶段才更新当前场景
    if (scene_ && trans_ != Trans::Switch) {
        scene_->Update();
    }
}
void SceneManager::Draw() {
    // 先画场景（Switch 阶段 scene_ 可能为 nullptr）
    if (scene_) { scene_->Draw(); }

    // 再画黑幕在最上层（确保每帧都能覆盖）
    ID3D12GraphicsCommandList* cl = DirectXCommon::GetInstance()->GetCommandList();
    Sprite::PreDraw(cl);
    if (overlay_ && overlayAlpha_ > 0.0f) {
        // （可选）防窗口尺寸改变：每帧更新大小
        overlay_->SetSize({ (float)WinApp::kWindowWidth, (float)WinApp::kWindowHeight });
        overlay_->Draw();
    }
    if (trans_ == Trans::Switch && showLoadingThisSwitch_ && loadingSprite_) {
        loadingSprite_->Draw();
    }
    Sprite::PostDraw();
}
