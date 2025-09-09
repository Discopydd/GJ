#pragma once
#include "KamataEngine.h"
#include "IScene.h"
#include "../map/MapChipField.h"
#include "../player/Player.h"
#include "../skydome/Skydome.h"
using namespace KamataEngine;


/// <summary>
/// ゲームシーン管理クラス
/// </summary>
class GameScene : public IScene {
public:
    GameScene(); // コンストラクタ
    ~GameScene(); // デストラクタ


    void Initialize() override; // 初期化
    void Update() override; // 毎フレーム更新
    void Draw() override; // 描画
    void Finalize() override;
    void SetStartMap(const std::string& path) { startMapPath_ = path; }
    bool IsSceneEnd() const { return exitToSelect_; }
    // ===== Raised（浮いているブロック） =====
    struct RaisedBlock {
        WorldTransform* wt = nullptr;
        uint32_t x = 0, y = 0;
        float highY = 0.0f; // 懸空状態の中心Y（高い位置）
        float lowY = 0.0f; // 落下状態の中心Y（地面と同じ高さ）
    };
    struct SpikeTile {
        WorldTransform* wt = nullptr;
        uint32_t x = 0, y = 0;
        bool active = false;   // 当前是否在“地面状态”（阻挡）
        bool animating = false;
        int dir = -1;          // -1: 下落, +1: 上升
        float highY = 0.0f;    // 高空位置
        float lowY = 0.0f;    // 落地位置

        bool lockToRaisedLow = false;
        float pairedRaisedLowY = 0.0f;
    };
private:
    DirectXCommon* dxCommon_ = nullptr;
    Input* input_ = nullptr;
    Camera camera_{};
    Model* model_ = nullptr;
    Model* obstacleModel_ = nullptr;
    Model* darkModel_ = nullptr;
    Model* darkObstacleModel_ = nullptr;
    Model* switchModel_ = nullptr;
    Model* goalModel_ = nullptr;
    Model* darkSwitchModel_ = nullptr;
    MapChipField mapChipField_; // マップチップデータ

    Skydome* skydome_ = nullptr;
    bool isDarkSky_ = false;
    // 生成されたブロック（地面などの常設）
    std::vector<std::vector<WorldTransform*>> mapBlocks_;


    // マップからブロックを生成
    void GenerateBlocks();


    Player* player_ = nullptr;

    std::vector<RaisedBlock> raisedBlocks_;
    std::vector<SpikeTile> spikeTiles_;

    // ===== Raised の往復アニメーション管理 =====
    bool animating_ = false; // 今まさに上下アニメ中か
    int animDir_ = -1; // -1: 下へ、+1: 上へ
    bool isLowered_ = false; // 直近の静止状態が「落下完了」なら true
    bool wasOnPortal_ = false; // 1フレーム前にポータル上だったか
    float moveSpeed_ = 0.25f; // 1フレームあたりのY移動量（元 dropSpeed_）

    bool playerLocked_ = false;  // 踩 Portal 后锁住玩家输入，动画全部完成时解锁

    // ==== 关卡（当前/下一关） ====
    std::string currentMapPath_ = "Resources/map.csv"; // 初始化时覆盖
    std::string nextMapPath_; // 若为空，则抵达 Goal 后重载当前关（等同通关重开）
    bool goalReached_ = false;


    void LoadLevel(const std::string& path);

    // ===== 步数限制 =====
    int initialSteps_ = 10;
    int remainingSteps_ = 10;    // 当前剩余步数
    bool lastPlayerMoving_ = false; // 上一帧玩家是否处于移动补间中

    std::string startMapPath_; // 从选关页传入

    bool exitToSelect_ = false;

    int PickInitialSteps(const std::string& path);

    // ===== Fade 遮罩（Sprite）=====
    Sprite* fadeSprite_ = nullptr;
    uint32_t fadeTexIndex_ = 0;     // white.png 的纹理索引（按你的 TextureManager API 替换）
    float   fadeAlpha_ = 0.0f;      // 0~1
    float   fadeSpeed_ = 0.03f;     // 调过渡速度
    bool    worldToggleInProgress_ = false; // 是否在做过渡
    bool    fadeOutPhase_ = true;           // true: 透明→黑；false: 黑→透明

    enum class FadeAction { ToggleWorld, ReloadToBright, ExitToSelect };
    FadeAction fadeAction_ = FadeAction::ToggleWorld;
};