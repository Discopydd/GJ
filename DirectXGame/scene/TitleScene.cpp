#include "TitleScene.h"
using namespace KamataEngine;
TitleScene::~TitleScene()
{
}
void TitleScene::Finalize() {
    delete titleSprite_;
    delete startSprite_;
    delete backSprite_;
}

void TitleScene::Initialize() {
    input_ = Input::GetInstance();
    dxCommon_ = DirectXCommon::GetInstance();
    titleTextureHandle_ = TextureManager::Load("GameTitle.png");
    titleSprite_ = Sprite::Create(titleTextureHandle_, { 0, 0 });
    backTextureHandle_ = TextureManager::Load("back.png");
    backSprite_ = Sprite::Create(backTextureHandle_, { 0, 0 });
    startTextureHandle_ = TextureManager::Load("Start.png");
    startSprite_ = Sprite::Create(startTextureHandle_, { 0, 360 });

    frameCount_ = 0;
}

void TitleScene::Update() {
    frameCount_++;

    // 上下に揺らす（sin波でY座標を変更）
    float offsetY = std::sin(frameCount_ * 0.05f) * 10.0f;
    titleSprite_->SetPosition({ 20, 20 + offsetY });

    // エンターキーでシーン切り替え
  bool go =
        input_->TriggerKey(DIK_SPACE)  ||
        input_->TriggerKey(DIK_RETURN) ||
        input_->IsTriggerMouse(0);

    if (go && sceneManager_) {
        auto* next = new LevelSelectScene();
        next->SetSceneManager(sceneManager_);          // 把 SM 指针传给下一场景
        sceneManager_->SetNextScene(next);             // ★ 交给 SceneManager 做全局淡出→切换→淡入
        return;
    }
}

void TitleScene::Draw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    Sprite::PreDraw(commandList);
    backSprite_->Draw();
    titleSprite_->Draw();
    // 60で割った余りが30以上なら描画（点滅）
    if ((frameCount_ % 60) >= 30) {
        startSprite_->Draw();
    }
    Sprite::PostDraw();
    dxCommon_->ClearDepthBuffer();
}

bool TitleScene::IsSceneEnd() const { 
    return isSceneEnd_;
}
