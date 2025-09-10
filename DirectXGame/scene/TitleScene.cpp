#include "TitleScene.h"
using namespace KamataEngine;

TitleScene::~TitleScene()
{
}
void TitleScene::Finalize() {
    delete titleSprite_;
    delete startSprite_;
    delete backSprite_;
    if (bgmVoice_) {
        KamataEngine::Audio::GetInstance()->StopWave(bgmVoice_);
    }

}

void TitleScene::Initialize() {
    input_ = Input::GetInstance();
    dxCommon_ = DirectXCommon::GetInstance();
    titleTextureHandle_ = TextureManager::Load("GameTitle.png");
    titleSprite_ = Sprite::Create(titleTextureHandle_, { 0, 0 });
    backTextureHandle_ = TextureManager::Load("back.png");
    backSprite_ = Sprite::Create(backTextureHandle_, { 0, 0 });
    startTextureHandle_ = TextureManager::Load("Start.png");
    startSprite_ = Sprite::Create(startTextureHandle_, { 0, 0 });
    seClick_ = KamataEngine::Audio::GetInstance()->LoadWave("se/decide.mp3");
    bgmHandle_ = KamataEngine::Audio::GetInstance()->LoadWave("se/select.mp3");
    bgmVoice_ = KamataEngine::Audio::GetInstance()->PlayWave(bgmHandle_, true, bgmVolume_);
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
        KamataEngine::Audio::GetInstance()->PlayWave(seClick_);
       fadingOut_ = true;  // 开始淡出
    }
    if (fadingOut_) {
        bgmVolume_ -= 0.02f;  // 每帧降低音量
        if (bgmVolume_ <= 0.0f) {
            bgmVolume_ = 0.0f;
            KamataEngine::Audio::GetInstance()->StopWave(bgmVoice_);
            // 完全静音后再切场景
            auto* next = new LevelSelectScene();
            next->SetSceneManager(sceneManager_);
            sceneManager_->SetNextScene(next);
            return;
        }
        KamataEngine::Audio::GetInstance()->SetVolume(bgmVoice_, bgmVolume_);
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
