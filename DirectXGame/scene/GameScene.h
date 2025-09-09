#pragma once
#include "KamataEngine.h"
#include "IScene.h"
#include "../map/MapChipField.h"
#include "../player/Player.h"
#include "../skydome/Skydome.h"
#include "LevelSelectScene.h"

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
    struct LocationMarker {
        WorldTransform* wt = nullptr;
        float baseY = 0.0f;   // 基准高度（位于1.5格处）
    };
    void SetSceneManager(SceneManager* sm) { sceneManager_ = sm; }
private:
    DirectXCommon* dxCommon_ = nullptr;
    Input* input_ = nullptr;
    Camera camera_{};
    SceneManager* sceneManager_ = nullptr;
    Model* model_ = nullptr;
    Model* obstacleModel_ = nullptr;
    Model* darkModel_ = nullptr;
    Model* darkObstacleModel_ = nullptr;
    Model* switchModel_ = nullptr;
    Model* goalModel_ = nullptr;
    Model* darkSwitchModel_ = nullptr;
    Model* locationModel_ = nullptr;
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

    // === 步数数字贴图 ===
    uint32_t stepDigitTex_[10]{};        // 0..9 纹理句柄
    std::vector<Sprite*> stepDigitSprites_; // 当前显示的“每一位”Sprite
    int lastStepsShown_ = -1;            // 上一帧显示的数值（变化才重建）

    // UI 参数（可调整）
    float stepDigitSize_ = 48.0f;     // 单个数字像素大小
    float stepDigitSpacing_ = -15.0f;      // 数字间距
    float stepDigitMargin_ = 10.0f;     // 距离右上角的边距

    // 生成/重建数字Sprites
    void RebuildStepDigits_(int value);

    // === Goal 上方的定位标记（会漂浮旋转） ===
    std::vector<LocationMarker> locationMarkers_;

    // 漂浮 / 旋转参数
    float locationBobPhase_ = 0.0f;   // 正弦相位
    float locationBobSpeed_ = 0.05f;  // 漂浮速度（调大更快）
    float locationBobAmpBlk_ = 0.5f;   // 漂浮幅度（以“格”为单位：0.5=半格，上下共1格）
    float locationRotSpeed_ = 0.03f;  // 每帧绕Y旋转（弧度）


    uint32_t asdTextureHandle_ = 0;
    Sprite* asdSprite_ = nullptr;

    // === 通关叠加图（clear.png） ===
    uint32_t clearTexHandle_ = 0;     // clear.png 纹理
    Sprite* clearSprite_ = nullptr;
    bool     showClear_ = false; // 正在显示 clear.png
    int      clearFrame_ = 0;     // 已显示的帧数
    static inline const int kClearShowFrames = 90; // 1.5s @60fps
};