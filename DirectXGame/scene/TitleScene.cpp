#include "TitleScene.h"
using namespace KamataEngine;
TitleScene::~TitleScene()
{
    delete titleSprite_;
    delete startSprite_;
}
void TitleScene::Initialize() {
    input_ = Input::GetInstance();
    dxCommon_ = DirectXCommon::GetInstance();
    titleTextureHandle_ = TextureManager::Load("GameTitle.png");
    titleSprite_ = Sprite::Create(titleTextureHandle_, {0, 0});
    startTextureHandle_ = TextureManager::Load("Start.png");
    startSprite_ = Sprite::Create(startTextureHandle_, {0, 360});
}

void TitleScene::Update() {
     frameCount_++;

    // 上下に揺らす（sin波でY座標を変更）
    float offsetY = std::sin(frameCount_ * 0.05f) * 10.0f;
    titleSprite_->SetPosition({ 20, 20 + offsetY });

    // エンターキーでシーン切り替え
    if (input_->TriggerKey(DIK_SPACE)) {
        isSceneEnd_ = true;
    }
}

void TitleScene::Draw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    Sprite::PreDraw(commandList);
    titleSprite_->Draw();
    // 60で割った余りが30以上なら描画（点滅）
    if ((frameCount_ % 60) >= 30) {
        startSprite_->Draw();
    }

    Sprite::PostDraw();
    dxCommon_->ClearDepthBuffer();
}

bool TitleScene::IsSceneEnd() {
    return isSceneEnd_;
}
