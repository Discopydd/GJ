#pragma once
#include <vector>
#include <string>
#include "KamataEngine.h"
#include "SceneManager.h" 
#include "GameScene.h"
#include "IScene.h"
class LevelSelectScene : public IScene {
public:
    void Initialize()override;
    void Update()override;
    void Draw()override;
    void Finalize()override;

    // main.cpp 会用到
    bool IsSceneEnd() const { return decided_; }
    std::string GetSelectedMap() const { return selectedMap_; }
    void SetSceneManager(SceneManager* sm) { sceneManager_ = sm; }
private:
    KamataEngine::Input* input_ = nullptr;
    KamataEngine::DirectXCommon* dxCommon_ = nullptr;
    std::vector<std::string> levels_;
    size_t selected_ = 0;       // 用 size_t 避免 C4267
    bool decided_ = false;      // 选关确认
    std::string selectedMap_;   // 确认后的地图路径

    uint32_t backTextureHandle_ = 0;
    KamataEngine::Sprite* backSprite_ = nullptr;

    // ===== 数字关卡按钮 =====
    struct LevelButton {
        KamataEngine::Sprite* sprite = nullptr;
        std::string mapPath;
        KamataEngine::Vector2 pos;   // 左上角
        KamataEngine::Vector2 size;  // 宽高
        uint32_t tex = 0;
    };
    std::vector<LevelButton> buttons_;

    // 布局参数（可调整）
    KamataEngine::Vector2 btnSize_ = { 162.0f, 162.0f };
    float gap_ = 128.0f;  // 按钮间距
    float margin_ = 128.0f;  // 边距
    int   cols_ = 4;      // 每行按钮个数

    void BuildButtons_();   // 生成/重建按钮
    void DestroyButtons_(); // 释放

    SceneManager* sceneManager_ = nullptr;

    uint32_t seClick_ = 0;
};
