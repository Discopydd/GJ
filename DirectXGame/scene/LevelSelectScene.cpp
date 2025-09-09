#include "LevelSelectScene.h"
#include <imgui.h>
#include <dinput.h> // DIK_UP, DIK_DOWN, DIK_RETURN
using namespace KamataEngine;

void LevelSelectScene::Initialize() {
    input_ = Input::GetInstance();
    dxCommon_ = DirectXCommon::GetInstance();
    levels_ = {
        "Resources/map/map.csv",
        "Resources/map/map_a.csv",
        "Resources/map/map_b.csv",
        "Resources/map/map_c.csv",
        "Resources/map/map_d.csv",
        "Resources/map/map_e.csv",
        "Resources/map/map_f.csv",
        "Resources/map/map_g.csv",
    };
    selected_ = 0;
    decided_ = false;
    selectedMap_.clear();

    backTextureHandle_ = TextureManager::Load("back.png");
    backSprite_ = Sprite::Create(backTextureHandle_, { 0, 0 });

    BuildButtons_();

}
void LevelSelectScene::Finalize() {
    DestroyButtons_();
    if (backSprite_) { delete backSprite_; backSprite_ = nullptr; }
}


void LevelSelectScene::Update() {
    if (levels_.empty()) return;

    // --- 鼠标命中 + 点击 ---
    Vector2 mp = input_->GetMousePosition();     // 窗口像素坐标
    bool clicked = input_->IsTriggerMouse(0);    // 左键点击瞬间

    for (size_t i = 0; i < buttons_.size(); ++i) {
        auto& b = buttons_[i];

        bool hover =
            (mp.x >= b.pos.x) && (mp.x <= b.pos.x + b.size.x) &&
            (mp.y >= b.pos.y) && (mp.y <= b.pos.y + b.size.y);

        if (hover && clicked && sceneManager_) {
            auto* next = new GameScene();
            next->SetStartMap(b.mapPath);
            next->SetSceneManager(sceneManager_);
            sceneManager_->SetNextScene(next);   // ★ 全局淡出→切→淡入
            return;
        }
    }
}


void LevelSelectScene::Draw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    Sprite::PreDraw(commandList);
    backSprite_->Draw();
    for (auto& b : buttons_) {
        b.sprite->Draw();
    }
    Sprite::PostDraw();
    dxCommon_->ClearDepthBuffer();
}
void LevelSelectScene::DestroyButtons_() {
    for (auto& b : buttons_) { delete b.sprite; }
    buttons_.clear();
}
void LevelSelectScene::BuildButtons_() {
    DestroyButtons_();

    // 网格布局：从左上角开始
    float startX = margin_;
    float startY = margin_;

    for (size_t i = 0; i < levels_.size(); ++i) {
        // 数字图片：放在 Resources/levels/1.png, 2.png, ..., 9.png（可自行扩展）
        uint32_t handle = TextureManager::Load("levelnum/" + std::to_string(i + 1) + ".png");

        int row = static_cast<int>(i) / cols_;
        int col = static_cast<int>(i) % cols_;
        Vector2 pos = { startX + col * (btnSize_.x + gap_),
                        startY + row * (btnSize_.y + gap_) };

        auto* sp = Sprite::Create(handle, pos);
        sp->SetAnchorPoint({ 0.0f, 0.0f });
        sp->SetSize(btnSize_);

        LevelButton b;
        b.sprite = sp;
        b.mapPath = levels_[i];
        b.pos = pos;
        b.size = btnSize_;
        b.tex = handle;
        buttons_.push_back(b);
    }
}