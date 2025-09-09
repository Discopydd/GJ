#include "GameScene.h"
#include <cassert>
#include <algorithm>
#include <imgui.h>
using namespace KamataEngine;
using namespace KamataEngine::MathUtility;

// ワールド座標をスクリーン座標に変換
static Vector2 WorldToScreen(const Vector3& worldPos, const Matrix4x4& view, const Matrix4x4& proj, float windowWidth, float windowHeight) {
    Matrix4x4 vpMatrix = view * proj;
    Vector3 ndcPos = TransformCoord(worldPos, vpMatrix);
    Vector2 screenPos;
    screenPos.x = (ndcPos.x * 0.5f + 0.5f) * windowWidth;
    screenPos.y = (1.0f - (ndcPos.y * 0.5f + 0.5f)) * windowHeight;
    return screenPos;
}

// 既存の WorldTransform* を安全に破棄
static void DisposeMapBlocks(std::vector<std::vector<WorldTransform*>>& mapBlocks) {
    for (auto& row : mapBlocks) {
        for (auto* wt : row) {
            delete wt;
        }
        row.clear();
    }
    mapBlocks.clear();
}
static void DisposeRaised(std::vector<GameScene::RaisedBlock>& raised) {
    for (auto& rb : raised) {
        delete rb.wt;
        rb.wt = nullptr;
    }
    raised.clear();
}
static void DisposeSpikes(std::vector<GameScene::SpikeTile>& spikes) {
for (auto& st : spikes) { delete st.wt; st.wt = nullptr; }
spikes.clear();
}
static void DisposeLocationMarkers(std::vector<GameScene::LocationMarker>& marks) {
    for (auto& m : marks) { delete m.wt; m.wt = nullptr; }
    marks.clear();
}
// マップからブロックを生成
void GameScene::GenerateBlocks() {
    // 以前の生成物を破棄（リーク防止）
    DisposeMapBlocks(mapBlocks_);
    DisposeRaised(raisedBlocks_);
    DisposeSpikes(spikeTiles_);
    DisposeLocationMarkers(locationMarkers_);
    mapBlocks_.resize(mapChipField_.numBlockVertical_);

    for (uint32_t y = 0; y < mapChipField_.numBlockVertical_; y++) {
        mapBlocks_[y].resize(mapChipField_.numBlockHorizontal_, nullptr);

        for (uint32_t x = 0; x < mapChipField_.numBlockHorizontal_; x++) {
            MapChipType type = mapChipField_.GetMapChipTypeByIndex(x, y);
            if (type == MapChipType::kBlock || type == MapChipType::kPortal || type == MapChipType::kRaised || type == MapChipType::kSpike || type == MapChipType::kGoal || type == MapChipType::kRaisedSpike) {
                auto* wt = new WorldTransform();
                wt->Initialize();
                Vector3 pos2D = mapChipField_.GetMapChipPositionByIndex(x, y);
                float tileHalf = MapChipField::kBlockHeight * 0.5f;
                wt->translation_ = { pos2D.x, tileHalf, pos2D.y };
                mapBlocks_[y][x] = wt;

                if (type == MapChipType::kRaised) {
                    auto* raised = new WorldTransform();
                    raised->Initialize();

                    float highY = tileHalf + MapChipField::kBlockHeight; // 懸空の高さ
                    float lowY = tileHalf;                               // 地面の高さ

                    raised->translation_ = { pos2D.x, highY, pos2D.y };
                    raisedBlocks_.push_back(RaisedBlock{ raised, x, y, highY, lowY });

                    // 初期は「上に居る」状態（＝上昇完了）とみなす
                    isLowered_ = false;
                }
                else if (type == MapChipType::kSpike) {
                    auto* spike = new WorldTransform();
                    spike->Initialize();
                    float lowY = tileHalf + MapChipField::kBlockHeight;                       // 地面中心
                    float highY = lowY + MapChipField::kBlockHeight * 5.0f; // 高空5格（可调）

                    // 初始：在高空（隐藏）
                    spike->translation_ = { pos2D.x, highY, pos2D.y };
                    spikeTiles_.push_back(SpikeTile{ spike, x, y, false, false, -1, highY, lowY });

                    // 初始地图格子可通过
                    mapChipField_.SetMapChipTypeByIndex(x, y, MapChipType::kBlock);
                }
                else if (type == MapChipType::kRaisedSpike) {
                    // Raised：初始在高处
                    auto* raised = new WorldTransform();
                    raised->Initialize();
                    float highY = tileHalf + MapChipField::kBlockHeight; // 悬空
                    float lowY = tileHalf;                               // 落地中心
                    raised->translation_ = { pos2D.x, highY, pos2D.y };
                    raisedBlocks_.push_back(RaisedBlock{ raised, x, y, highY, lowY });
                    isLowered_ = false;

                    // Spike：初始在更高的高空（可见，不阻挡）
                    auto* spike = new WorldTransform();
                    spike->Initialize();
                    float spikeLowY = tileHalf + MapChipField::kBlockHeight;         // Spike 自己的“地面中心”
                    float spikeHighY = spikeLowY + MapChipField::kBlockHeight * 5.0f; // 高空
                    spike->translation_ = { pos2D.x, spikeHighY, pos2D.y };

                    SpikeTile st{};
                    st.wt = spike; st.x = x; st.y = y;
                    st.active = false;         // 初始不在地面，不阻挡
                    st.animating = false;
                    st.dir = -1;               // 首次触发时会“下降”
                    st.highY = spikeHighY;
                    st.lowY = spikeLowY;      // 常规用不到，但保留
                    st.lockToRaisedLow = true;
                    st.pairedRaisedLowY = lowY; // ★ 绑定到该格 Raised 的“落地中心Y”
                    spikeTiles_.push_back(st);
                }
                else if (type == MapChipType::kGoal) {
                    // 普通的地块 WT 你已创建；再创建定位标记 WT
                    auto* loc = new WorldTransform();
                    loc->Initialize();

                    float locationTileHalf = MapChipField::kBlockHeight * 0.5f;

                    // 基准高度：比地面中心高 1.5 格（介于1~2格之间）
                    float baseY = locationTileHalf + MapChipField::kBlockHeight * 1.5f;
                    loc->translation_ = { pos2D.x, baseY, pos2D.y };
                    loc->rotation_ = { 0.0f, 0.0f, 0.0f };
                    locationMarkers_.push_back(LocationMarker{ loc, baseY });
                }
            }
        }
    }
    if (!model_)            model_ = Model::CreateFromOBJ("cube", true);
    if (!obstacleModel_)    obstacleModel_ = Model::CreateFromOBJ("obstacle", true);
    if (!switchModel_)      switchModel_ = Model::CreateFromOBJ("switch", true);
    if (!goalModel_)        goalModel_ = Model::CreateFromOBJ("goal", true);
    if (!darkModel_)        darkModel_ = Model::CreateFromOBJ("darkCube", true);
    if (!darkObstacleModel_)darkObstacleModel_ = Model::CreateFromOBJ("darkObstacle", true);
    if (!darkSwitchModel_)  darkSwitchModel_ = Model::CreateFromOBJ("darkSwitch", true);
    if (!locationModel_) locationModel_ = Model::CreateFromOBJ("location", true);
}

void GameScene::LoadLevel(const std::string& path)
{
    // 重新加载地图并重建方块
    currentMapPath_ = path;
    initialSteps_ = PickInitialSteps(currentMapPath_);
    mapChipField_.LoadMapChipCsv(currentMapPath_);
    GenerateBlocks();
    FitCameraToWholeMap45(1.0f, 60.0f, 180.0f);

    // 重置玩家到起点（这里仍放最上行左侧）
    uint32_t topY = (mapChipField_.numBlockVertical_ > 0) ? (mapChipField_.numBlockVertical_ - 1) : 0;
    float blockTopY = MapChipField::kBlockHeight;
    float playerCenterY = blockTopY + player_->GetHeight() * 0.5f;
    player_->SetByTileIndex(mapChipField_, 0, topY, playerCenterY);
    player_->ResetOrientation();

    // 恢复状态
    animating_ = false; isLowered_ = false; wasOnPortal_ = false; playerLocked_ = false; goalReached_ = false;

    // 重置步数
    remainingSteps_ = initialSteps_;
    lastPlayerMoving_ = false;
    exitToSelect_ = false;
}

int GameScene::PickInitialSteps(const std::string& path)
{
    if (path.find("Resources/map/map_a.csv") != std::string::npos) return 10;
    if (path.find("Resources/map/map_b.csv") != std::string::npos) return 14;
    if (path.find("Resources/map/map_c.csv") != std::string::npos) return 27;
    if (path.find("Resources/map/map_d.csv") != std::string::npos) return 36;
    if (path.find("Resources/map/map_e.csv") != std::string::npos) return 20;
    if (path.find("Resources/map/map_f.csv") != std::string::npos) return 30;
    if (path.find("Resources/map/map_g.csv") != std::string::npos) return 40;

    // 兼容你默认的单图用法
    if (path.find("Resources/map.csv") != std::string::npos)   return 10;

    // 未匹配：给个保底值
    return 20;
}

void GameScene::RebuildStepDigits_(int value)
{
      // 清理旧的
    for (auto* s : stepDigitSprites_) { delete s; }
    stepDigitSprites_.clear();

    if (value < 0) value = 0;

    // 把数值转为字符串（至少显示一位）
    std::string s = std::to_string(value);

    // 右上角排版（不改变锚点，直接算坐标）
    float xRight = (float)KamataEngine::WinApp::kWindowWidth  - stepDigitMargin_;
    float yTop   = stepDigitMargin_;  // 顶部边距
    // 逐位从右往左放
    for (int i = (int)s.size() - 1; i >= 0; --i) {
        int d = s[i] - '0';
        float x = xRight - (float)(s.size() - 1 - i) * (stepDigitSize_ + stepDigitSpacing_);
        auto* sp = KamataEngine::Sprite::Create(stepDigitTex_[d], { x - stepDigitSize_, yTop }); // 以左上角为参考
        sp->SetAnchorPoint({0.0f, 0.0f});
        sp->SetSize({ stepDigitSize_, stepDigitSize_ });
        sp->SetColor({1,1,1,1});
        stepDigitSprites_.push_back(sp);
    }

    lastStepsShown_ = value;
}

void GameScene::FitCameraToWholeMap45(float marginBlocks, float pitchDeg, float yawDeg)
{
    // 地图整体尺寸（单位：世界坐标）
    const float mapW = mapChipField_.numBlockHorizontal_ * MapChipField::kBlockWidth;
    const float mapH = mapChipField_.numBlockVertical_ * MapChipField::kBlockHeight;

    // 地图中心（X-Z 平面），Y 取地面上方一点
    const float groundY = MapChipField::kBlockHeight * 0.5f; // 地砖中心高度
    const Vector3 mapCenter = {
        mapW * 0.5f,
        groundY,
        mapH * 0.5f
    };

    // 以 45° 俯视等距角度（Pitch=-45°, Yaw=+45°）
    const float deg2rad = 3.1415926535f / 180.0f;
    const float pitch = pitchDeg * deg2rad;
    const float yaw = yawDeg * deg2rad;

    camera_.rotation_ = { pitch, yaw, 0.0f };

    // 估个“需要的距离”，按对角线 + 留白来算
    const float marginW = marginBlocks * MapChipField::kBlockWidth;
    const float marginH = marginBlocks * MapChipField::kBlockHeight;
    const float wantW = mapW + marginW * 2.0f;
    const float wantH = mapH + marginH * 2.0f;

    // 因为我们固定 45° 俯视，经验上这样取距离比较稳：
    // 让距离与较长边成比例，再乘个系数即可。
    const float major = (std::max)(wantW, wantH);
    const float distance = major * 1.2f; // 可按需要调系数（1.1～1.6）

    // 由 yaw/pitch 反推出相机相对中心的方向（单位向量）
    // 朝向是从相机指向中心，因此相机位置 = 中心 - dir * distance
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);

    // 视线方向（右手系，X 前右，Y 上，Z 前？你的世界里 Z 是“纵向”）
    // 这里构造一个标准前向：在 XZ 平面朝 yaw，再向下俯 pitch
    Vector3 forward = {
        cp * sy,   // x
        -sp,       // y（向下为负）
        cp * cy    // z
    };

    // 相机位置：在 forward 的反方向拉开 distance
    camera_.translation_ = {
        mapCenter.x - forward.x * distance,
        mapCenter.y - forward.y * distance,
        mapCenter.z - forward.z * distance
    };

    // 轻微上抬，避免地面裁切（可选）
    camera_.translation_.y += MapChipField::kBlockHeight * 0.5f;

    camera_.UpdateMatrix();
}

GameScene::GameScene() {}

GameScene::~GameScene() {
   Finalize();
}
void GameScene::Finalize() {
    delete player_;          player_ = nullptr;
    delete skydome_;         skydome_ = nullptr;
    delete model_;           model_ = nullptr;        // cube
    delete obstacleModel_;   obstacleModel_ = nullptr;
    delete goalModel_;       goalModel_ = nullptr;        // cube
    delete switchModel_;   switchModel_ = nullptr;
    delete darkModel_;          darkModel_ = nullptr;
    delete darkObstacleModel_;  darkObstacleModel_ = nullptr;
    delete darkSwitchModel_;  darkSwitchModel_ = nullptr;
    delete fadeSprite_; fadeSprite_ = nullptr;
    delete locationModel_; locationModel_ = nullptr;
    delete clearSprite_; clearSprite_ = nullptr;
    for (auto* s : stepDigitSprites_) { delete s; }
    stepDigitSprites_.clear();
    // 生成物破棄
    DisposeRaised(raisedBlocks_);
    DisposeMapBlocks(mapBlocks_);
    DisposeSpikes(spikeTiles_);
    DisposeLocationMarkers(locationMarkers_);
}
// 初期化
void GameScene::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    input_ = Input::GetInstance();

    camera_.Initialize();
    camera_.translation_ = { 2.5f, 25.0f, 20.0f };
    camera_.rotation_ = { 2.2f, 0.0f, 0.0f };
    camera_.UpdateMatrix();
    skydome_ = new Skydome();
    skydome_->Initialize(&camera_, "Skydome");
    // マップ読み込み
    if (!startMapPath_.empty()) {
        currentMapPath_ = startMapPath_;
    }
    else {
        currentMapPath_ = "Resources/map.csv";  // 作为默认值
    }
    initialSteps_ = PickInitialSteps(currentMapPath_);
    mapChipField_.LoadMapChipCsv(currentMapPath_);
    GenerateBlocks();
    FitCameraToWholeMap45(/*marginBlocks=*/1.0f, /*pitchDeg=*/60.0f, /*yawDeg=*/180.0f);
    // プレイヤー初期化
    player_ = new Player();
    player_->Initialize(&camera_, &mapChipField_, "player");

    uint32_t topY = (mapChipField_.numBlockVertical_ > 0) ? (mapChipField_.numBlockVertical_ - 1) : 0;
    float blockTopY = MapChipField::kBlockHeight;
    float playerCenterY = blockTopY + player_->GetHeight() * 0.5f;
    player_->SetByTileIndex(mapChipField_, 0, topY, playerCenterY);

    // === Fade 遮罩 Sprite ===
// 1) 加载纹理（按你的 TextureManager 接口来写）
    fadeTexIndex_ = TextureManager::GetInstance()->Load("white1x1.png");
    // 2) 创建 Sprite
    fadeSprite_ = Sprite::Create(fadeTexIndex_, { 0.0f, 0.0f });
    fadeSprite_->SetAnchorPoint({ 0.0f, 0.0f });
    fadeSprite_->SetPosition({ 0.0f, 0.0f });

    // ★ 使用 WinApp 的窗口常量
    fadeSprite_->SetSize({ (float)KamataEngine::WinApp::kWindowWidth,
                           (float)KamataEngine::WinApp::kWindowHeight });

    fadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f }); // 初始透明
    // 载入 0..9 贴图
    for (int d = 0; d < 10; ++d) {
        stepDigitTex_[d] = KamataEngine::TextureManager::Load("numbers/" + std::to_string(d) + ".png");
    }

    // 生成当前步数的数字Sprites
    RebuildStepDigits_(remainingSteps_);


    asdTextureHandle_ = TextureManager::Load("asd.png");
    asdSprite_ = Sprite::Create(asdTextureHandle_, { 0, 0 });

    // === clear.png 叠加图 ===
    clearTexHandle_ = TextureManager::Load("clear.png"); // 路径按你的资源目录
    clearSprite_ = Sprite::Create(clearTexHandle_, { 0, -100 });
    showClear_ = false;
    clearFrame_ = 0;

    // 状態初期化
    animating_ = false;
    animDir_ = -1;
    isLowered_ = false; // Raised が存在すれば初期は上にいる
    wasOnPortal_ = false;
    remainingSteps_ = initialSteps_;
    lastPlayerMoving_ = false;

    exitToSelect_ = false;
}

// 毎フレーム更新
void GameScene::Update() {
    // ===== 相机调试 UI =====
    ImGui::Begin("Camera Controller");
    static float pos[3], rot[3];
    pos[0] = camera_.translation_.x; pos[1] = camera_.translation_.y; pos[2] = camera_.translation_.z;
    rot[0] = camera_.rotation_.x;    rot[1] = camera_.rotation_.y;    rot[2] = camera_.rotation_.z;
    if (ImGui::DragFloat3("Position", pos, 0.1f)) { camera_.translation_ = { pos[0], pos[1], pos[2] }; }
    if (ImGui::DragFloat3("Rotation", rot, 0.01f)) { camera_.rotation_ = { rot[0], rot[1], rot[2] }; }
    ImGui::End();
    camera_.UpdateMatrix();
    if (skydome_) skydome_->Update();
    // ===== 玩家：只有未上锁时才允许更新（避免动画期间操作）=====
    if (player_) {
        // 记录更新前的“是否在移动”状态，用于检测 false->true 的边沿
        bool wasMoving = player_->IsMoving();  // 需要 Player.h 的 IsMoving():contentReference[oaicite:3]{index=3}

        if (!playerLocked_) {
            // 步数门控：
            // 1) 若还有步数，允许接收输入并可能开始新的移动；
            // 2) 若步数为0，但玩家“还在补间中”，也要继续Update以完成该次移动，避免卡在半格。
            if (remainingSteps_ > 0 || wasMoving) {
                player_->Update();  // 可能会在本帧把 isMoving_ 置为 true:contentReference[oaicite:4]{index=4}
            }
        }

        // 记录更新后的“是否在移动”
        bool nowMoving = player_->IsMoving();

        // 扣步：当且仅当从 未移动 -> 移动 的瞬间 扣1步
        if (!wasMoving && nowMoving) {
            remainingSteps_ = (std::max)(0, remainingSteps_ - 1);
        }

        // 保存当前状态供下一帧比较
        lastPlayerMoving_ = nowMoving;
    }
    // ===== 快捷键：返回关卡选择 =====
    if (input_->TriggerKey(DIK_TAB)) {
        auto* next = new LevelSelectScene();
        next->SetSceneManager(sceneManager_); 
        sceneManager_->SetNextScene(next);
        return;
    }

    // === （可选）步数显示：ImGui ===
    ImGui::Begin("Game Info");
    ImGui::Text("Steps: %d / %d", remainingSteps_, initialSteps_);
    ImGui::End();
    // ===== 按 R 重新开始（使用过渡）=====
    if (input_->TriggerKey(DIK_R)) {
        // 启动黑幕过渡
        if (!worldToggleInProgress_) {
            worldToggleInProgress_ = true;
            fadeOutPhase_ = true;
            fadeAction_ = FadeAction::ReloadToBright;  // ★ 关键
            playerLocked_ = true;                      // 防误操作
        }
        return;                                  // 本帧到此为止
    }
    // ===== 检查是否“完全进入”Portal（必须停稳在该格中心）=====
    bool onPortalTile = false, fullyInsidePortal = false;
    bool onGoalTile = false, fullyInsideGoal = false;
    if (player_) {
        uint32_t px = player_->TileX();
        uint32_t py = player_->TileY();
        MapChipType t = mapChipField_.GetMapChipTypeByIndex(px, py);
        onPortalTile = (t == MapChipType::kPortal);
        onGoalTile = (t == MapChipType::kGoal);
        // 条件：在 Portal 格 && 玩家已停止补间
        if (onPortalTile && !player_->IsMoving()) {
            fullyInsidePortal = true;

            // （可选更严谨几何判定）
            // const auto& p = player_->GetWorldPosition();
            // auto rect = mapChipField_.GetRectByIndex(px, py);
            // const float eps = 1e-4f;
            // fullyInsidePortal &= (p.x > rect.left - eps && p.x < rect.right + eps &&
            //                       p.y > rect.bottom - eps && p.y < rect.top + eps);
        }
        if (onGoalTile && !player_->IsMoving()) {
            fullyInsideGoal = true;
        }
    }

    // ===== Portal 边缘触发（从“未完全进入”->“完全进入”的瞬间）=====
    if (fullyInsidePortal && !wasOnPortal_) {
        bool started = false;

        // 1) Raised：若当前没在动，切换方向并启动
        if (!animating_ && !raisedBlocks_.empty()) {
            animating_ = true;
            animDir_ = isLowered_ ? +1 : -1;   // 地面→上升；高处→下落
            started = true;
        }

        // 2) Spike：逐个切换（地面→上升；高空→下落）
        for (auto& st : spikeTiles_) {
            if (st.animating) continue;          // 正在动的别打断
            st.animating = true;
            st.dir = st.active ? +1 : -1;        // active(在地面)=上升；否则=下落
            started = true;
        }

        if (started) {
            playerLocked_ = true;                // 触发即上锁
            if (!worldToggleInProgress_) {
                worldToggleInProgress_ = true;
                fadeOutPhase_ = true;
                // 不立即翻转，等“全黑”时再翻
                fadeAction_ = FadeAction::ToggleWorld;
            }
        }
    }
    wasOnPortal_ = fullyInsidePortal;            // 记录“完全在 Portal”的状态
    // ===== Goal 触发：完全进入 Goal 即通关/切关 =====
    if (fullyInsideGoal && !goalReached_) {
        goalReached_ = true;
        playerLocked_ = true;
        // 1) 先显示通关叠加图 1.5 秒（不清屏）
        showClear_ = true;
        clearFrame_ = 0;

        // 2) 先不切场景；计时结束后再交给 SceneManager 做带 Fade 的切换
        return;
    }
    // ===== 推进 Raised 动画 =====
    if (animating_) {
        bool allDone = true;
        for (auto& rb : raisedBlocks_) {
            if (!rb.wt) continue;
            const float targetY = (animDir_ < 0) ? rb.lowY : rb.highY;
            const float step = moveSpeed_ * ((animDir_ < 0) ? -1.0f : 1.0f);
            rb.wt->translation_.y += step;

            if ((animDir_ < 0 && rb.wt->translation_.y <= targetY) ||
                (animDir_ > 0 && rb.wt->translation_.y >= targetY)) {
                rb.wt->translation_.y = targetY;
            }
            else {
                allDone = false;
            }
        }
        if (allDone) {
            animating_ = false;
            isLowered_ = (animDir_ < 0);
            // 与你现有语义一致：落地→Block（可走地面），升起→Raised（悬空）
            for (auto& rb : raisedBlocks_) {
                mapChipField_.SetMapChipTypeByIndex(
                    rb.x, rb.y, isLowered_ ? MapChipType::kBlock : MapChipType::kRaised
                );
            }
        }
    }
    // === 位置标记动画（上下浮动 + 旋转）===
    locationBobPhase_ += locationBobSpeed_;
    const float amp = locationBobAmpBlk_ * MapChipField::kBlockHeight; // 将“格”换算成世界单位
    for (auto& m : locationMarkers_) {
        if (!m.wt) continue;
        // 上下浮动：围绕 baseY 在 [baseY - amp, baseY + amp] 之间
        m.wt->translation_.y = m.baseY + std::sinf(locationBobPhase_) * amp;
        // 绕 Y 轴匀速旋转
        m.wt->rotation_.y += locationRotSpeed_;
    }

    // ===== 推进 Spike 动画 =====
    bool anySpikeAnimating = false;
    for (auto& st : spikeTiles_) {
        if (!st.animating || !st.wt) continue;
        anySpikeAnimating = true;

        float targetY;
        if (st.dir < 0) {
            if (st.lockToRaisedLow) {
                // ★ 关键：下降到“Raised 的正上方一格”（中心相差一个方块高度）
                targetY = st.pairedRaisedLowY + MapChipField::kBlockHeight;
            }
            else {
                targetY = st.lowY;
            }
        }
        else {
            targetY = st.highY;
        }

        const float step = moveSpeed_ * ((st.dir < 0) ? -1.0f : 1.0f);
        st.wt->translation_.y += step;

        if ((st.dir < 0 && st.wt->translation_.y <= targetY) ||
            (st.dir > 0 && st.wt->translation_.y >= targetY)) {
            st.wt->translation_.y = targetY;
            st.animating = false;

            // 只有落地时阻挡；回到高空可通过
            st.active = (st.dir < 0);
            mapChipField_.SetMapChipTypeByIndex(
                st.x, st.y, st.active ? MapChipType::kSpike : MapChipType::kBlock
            );
        }
    }
    // ===== 世界切换过渡（淡入→切换→淡出） =====
    if (worldToggleInProgress_) {
        if (fadeOutPhase_) {
            fadeAlpha_ += fadeSpeed_;
            if (fadeAlpha_ >= 1.0f) {
                fadeAlpha_ = 1.0f;

                switch (fadeAction_) {
                case FadeAction::ToggleWorld:
                    isDarkSky_ = !isDarkSky_;
                    if (skydome_) { skydome_->SetModel(isDarkSky_ ? "darkSkydome" : "Skydome"); }
                    break;

                case FadeAction::ReloadToBright:
                    isDarkSky_ = false;
                    if (skydome_) { skydome_->SetModel("Skydome"); }
                    LoadLevel(currentMapPath_);
                    // 维持黑幕，准备淡出
                    fadeAlpha_ = 1.0f;
                    if (fadeSprite_) { fadeSprite_->SetColor({ 0,0,0,1 }); }
                    break;

                case FadeAction::ExitToSelect:                 // ★ 新增
                    exitToSelect_ = true;                      // 通知主循环切到选关
                    // 如果你希望“先黑屏再切场景”，这里可直接 return; 让主循环下一帧切走
                    // 若想在本场景自己淡回，则保持现逻辑进入淡出阶段
                    break;
                }
                fadeAction_ = FadeAction::ToggleWorld; // 复位为默认（可选）
                fadeOutPhase_ = false;
            }
        }
        else {
            // 黑 -> 透明
            fadeAlpha_ -= fadeSpeed_;
            if (fadeAlpha_ <= 0.0f) {
                fadeAlpha_ = 0.0f;
                worldToggleInProgress_ = false;
                playerLocked_ = false; // 过渡结束解锁
            }
        }
    }
    // ===== 动画完成→解锁 =====
    const bool anyAnimatingNow = animating_ || anySpikeAnimating;
    if (playerLocked_ && !anyAnimatingNow && !worldToggleInProgress_) {
        playerLocked_ = false;
    }
    // ===== 通关叠加：计时 1.5 秒后切回选关（SceneManager 自带 Fade）=====
    if (showClear_) {
        clearFrame_++;
        if (clearFrame_ >= kClearShowFrames) {
            showClear_ = false;  // 关闭叠加
            clearFrame_ = 0;

            if (sceneManager_) {
                auto* next = new LevelSelectScene();
                next->SetSceneManager(sceneManager_);

                // 使用 SceneManager 的过渡（淡出→切场→淡入）
                sceneManager_->SetNextScene(next);
            }
            else {
                // 兼容无 SceneManager 的旧逻辑
                exitToSelect_ = true;
            }
            return; // 本帧结束（避免继续处理别的逻辑）
        }
    }

    if (lastStepsShown_ != remainingSteps_) {
        RebuildStepDigits_(remainingSteps_);
    }
}


// 描画
void GameScene::Draw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    // 背景スプライト描画
    Sprite::PreDraw(commandList);
    Sprite::PostDraw();
    dxCommon_->ClearDepthBuffer();

    // 3Dオブジェクト描画
    Model::PreDraw();
    if (skydome_) skydome_->Draw();

    Model* tileModel   = isDarkSky_ ? darkModel_         : model_;
    Model* obstModel = isDarkSky_ ? darkObstacleModel_ : obstacleModel_;

    Model* switchNow = (isDarkSky_ && darkSwitchModel_) ? darkSwitchModel_ : switchModel_;
    for (uint32_t y = 0; y < mapBlocks_.size(); y++) {
        for (uint32_t x = 0; x < mapBlocks_[y].size(); x++) {
            if (mapBlocks_[y][x]) {
                mapBlocks_[y][x]->UpdateMatrix();

                MapChipType type = mapChipField_.GetMapChipTypeByIndex(x, y);

                if (type == MapChipType::kPortal) {
                    // 2 → 用 Switch 模型
                    switchNow->Draw(*mapBlocks_[y][x], camera_);
                }
                else if (type == MapChipType::kGoal) {
                    // 5 → 用 Goal 模型
                    goalModel_->Draw(*mapBlocks_[y][x], camera_);
                }
                else {
                    // 其他（普通方块等）
                    tileModel->Draw(*mapBlocks_[y][x], camera_);
                }
            }
        }
    }
    for (auto& rb : raisedBlocks_) {
        if (rb.wt) {
            rb.wt->UpdateMatrix();
            obstModel->Draw(*rb.wt, camera_);
        }
    }
    // Spike（仅在显示时绘制 obstacle）
    for (auto& st : spikeTiles_) {
        if (st.wt) {
            st.wt->UpdateMatrix();
            obstModel->Draw(*st.wt, camera_);
        }
    }
    // === Goal 上方定位标记 ===
    if (locationModel_) {
        for (auto& m : locationMarkers_) {
            if (!m.wt) continue;
            m.wt->UpdateMatrix();
            locationModel_->Draw(*m.wt, camera_);
        }
    }
    if (player_) player_->Draw();
    Model::PostDraw();

    // 前景スプライト描画
    Sprite::PreDraw(commandList);
    // ここで Portal 上のUIなどを描く場合は WorldToScreen を活用
    for (auto* s : stepDigitSprites_) {
        s->Draw();
    }
    asdSprite_->Draw();
    if (showClear_ && clearSprite_) {
        clearSprite_->Draw();
    }
    if (fadeSprite_ && fadeAlpha_ > 0.0f) {
        fadeSprite_->SetColor({ 0,0,0,fadeAlpha_ });
        fadeSprite_->Draw();
    }
    Sprite::PostDraw();
}
