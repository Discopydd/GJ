#include "SceneManager.h"

using namespace KamataEngine;

SceneManager::SceneManager() {
    EnsureOverlay_();
}

SceneManager::~SceneManager() {
    if (scene_) { scene_->Finalize(); delete scene_; }
    if (pendingScene_) { delete pendingScene_; pendingScene_ = nullptr; }
    if (overlay_) { delete overlay_; overlay_ = nullptr; }
}

void SceneManager::EnsureOverlay_() {
    if (overlay_) return;
    uint32_t white = TextureManager::Load("white1x1.png");   // Resources/white1x1.png
    overlay_ = Sprite::Create(white, {0,0});
    overlay_->SetAnchorPoint({0,0});
    overlay_->SetSize({ (float)WinApp::kWindowWidth, (float)WinApp::kWindowHeight });
    overlay_->SetColor({0,0,0,overlayAlpha_});
}

void SceneManager::SetNextScene(IScene* scene) {
    // 不立即切，先存起来并进入 FadeOut
    if (pendingScene_) { delete pendingScene_; }
    pendingScene_ = scene;
    if (trans_ == Trans::Idle) {
        StartFadeOut_();
    }
}

void SceneManager::StartFadeOut_() {
    EnsureOverlay_();
    trans_ = Trans::FadeOut;
    // 若你想更慢：overlaySpeed_ = 0.03f;
}

void SceneManager::DoSwitch_() {
    // 删除旧场景，初始化新场景（此时屏幕已黑）
    if (scene_) { scene_->Finalize(); delete scene_; scene_ = nullptr; }
    scene_ = pendingScene_;
    pendingScene_ = nullptr;
    if (scene_) { scene_->Initialize(); }    // 即便耗时，上一帧已是全黑
    trans_ = Trans::FadeIn;                  // 切完准备淡入
}

void SceneManager::Update() {
    // === 过渡推进 ===
    switch (trans_) {
    case Trans::FadeOut:
        overlayAlpha_ += overlaySpeed_;
        if (overlayAlpha_ >= 1.0f) {
            overlayAlpha_ = 1.0f;
            trans_ = Trans::Switch;          // 下一步做真正切换
        }
        if (overlay_) overlay_->SetColor({0,0,0,overlayAlpha_});
        break;

    case Trans::Switch:
        DoSwitch_();                         // 同一帧或下一帧都行
        if (overlay_) overlay_->SetColor({0,0,0,overlayAlpha_}); // 仍保持全黑
        break;

    case Trans::FadeIn:
        overlayAlpha_ -= overlaySpeed_;
        if (overlayAlpha_ <= 0.0f) {
            overlayAlpha_ = 0.0f;
            trans_ = Trans::Idle;
        }
        if (overlay_) overlay_->SetColor({0,0,0,overlayAlpha_});
        break;

    case Trans::Idle:
        // 无过渡，正常进行
        break;
    }

    // === 场景更新（不在 Switch 阶段时才更新）===
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
    Sprite::PostDraw();
}
