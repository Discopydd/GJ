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

// マップからブロックを生成
void GameScene::GenerateBlocks() {
    // 以前の生成物を破棄（リーク防止）
    DisposeMapBlocks(mapBlocks_);
    DisposeRaised(raisedBlocks_);
    DisposeSpikes(spikeTiles_);
    mapBlocks_.resize(mapChipField_.numBlockVertical_);

    for (uint32_t y = 0; y < mapChipField_.numBlockVertical_; y++) {
        mapBlocks_[y].resize(mapChipField_.numBlockHorizontal_, nullptr);

        for (uint32_t x = 0; x < mapChipField_.numBlockHorizontal_; x++) {
            MapChipType type = mapChipField_.GetMapChipTypeByIndex(x, y);
            if (type == MapChipType::kBlock || type == MapChipType::kPortal || type == MapChipType::kRaised || type == MapChipType::kSpike) {
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
                    float lowY  = tileHalf + MapChipField::kBlockHeight;                       // 地面中心
                    float highY = lowY + MapChipField::kBlockHeight * 5.0f; // 高空5格（可调）

                    // 初始：在高空（隐藏）
                    spike->translation_ = { pos2D.x, highY, pos2D.y };
                    spikeTiles_.push_back(SpikeTile{ spike, x, y, false, false, -1, highY, lowY });

                    // 初始地图格子可通过
                    mapChipField_.SetMapChipTypeByIndex(x, y, MapChipType::kBlock);
                }
            }
        }
    }

    // モデル生成
    model_ = Model::CreateFromOBJ("cube", true);
    obstacleModel_ = Model::CreateFromOBJ("obstacle", true);
}

GameScene::GameScene() {}

GameScene::~GameScene() {
    // プレイヤー
    delete player_;
    player_ = nullptr;

    // 生成物破棄
    DisposeRaised(raisedBlocks_);
    DisposeMapBlocks(mapBlocks_);
    DisposeSpikes(spikeTiles_);
}

// 初期化
void GameScene::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    input_ = Input::GetInstance();

    camera_.Initialize();
    camera_.translation_ = { -10.0f, 20.0f, -20.0f };
    camera_.rotation_ = { 0.5f, 0.5f, 0.0f };
    camera_.UpdateMatrix();

    // マップ読み込み
    mapChipField_.LoadMapChipCsv("Resources/map.csv");
    GenerateBlocks();

    // プレイヤー初期化
    player_ = new Player();
    player_->Initialize(&camera_, &mapChipField_, "player");

    uint32_t topY = (mapChipField_.numBlockVertical_ > 0) ? (mapChipField_.numBlockVertical_ - 1) : 0;
    float blockTopY = MapChipField::kBlockHeight;
    float playerCenterY = blockTopY + player_->GetHeight() * 0.5f;
    player_->SetByTileIndex(mapChipField_, 0, topY, playerCenterY);

    // 状態初期化
    animating_   = false;
    animDir_     = -1;
    isLowered_   = false; // Raised が存在すれば初期は上にいる
    wasOnPortal_ = false;
}

// 毎フレーム更新
void GameScene::Update() {
    // ===== 相机调试 UI =====
    ImGui::Begin("Camera Controller");
    static float pos[3], rot[3];
    pos[0] = camera_.translation_.x; pos[1] = camera_.translation_.y; pos[2] = camera_.translation_.z;
    rot[0] = camera_.rotation_.x;    rot[1] = camera_.rotation_.y;    rot[2] = camera_.rotation_.z;
    if (ImGui::DragFloat3("Position", pos, 0.1f)) { camera_.translation_ = { pos[0], pos[1], pos[2] }; }
    if (ImGui::DragFloat3("Rotation", rot, 0.01f)) { camera_.rotation_    = { rot[0], rot[1], rot[2] }; }
    ImGui::End();
    camera_.UpdateMatrix();

    // ===== 玩家：只有未上锁时才允许更新（避免动画期间操作）=====
    if (!playerLocked_) {
        if (player_) player_->Update();
    }

    // ===== 检查是否“完全进入”Portal（必须停稳在该格中心）=====
    bool onPortalTile = false;
    bool fullyInsidePortal = false;
    if (player_) {
        uint32_t px = player_->TileX();
        uint32_t py = player_->TileY();
        MapChipType t = mapChipField_.GetMapChipTypeByIndex(px, py);
        onPortalTile = (t == MapChipType::kPortal);

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
    }

    // ===== Portal 边缘触发（从“未完全进入”->“完全进入”的瞬间）=====
    if (fullyInsidePortal && !wasOnPortal_) {
        bool started = false;

        // 1) Raised：若当前没在动，切换方向并启动
        if (!animating_ && !raisedBlocks_.empty()) {
            animating_ = true;
            animDir_   = isLowered_ ? +1 : -1;   // 地面→上升；高处→下落
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
        }
    }
    wasOnPortal_ = fullyInsidePortal;            // 记录“完全在 Portal”的状态

    // ===== 推进 Raised 动画 =====
    if (animating_) {
        bool allDone = true;
        for (auto& rb : raisedBlocks_) {
            if (!rb.wt) continue;
            const float targetY = (animDir_ < 0) ? rb.lowY : rb.highY;
            const float step    = moveSpeed_ * ((animDir_ < 0) ? -1.0f : 1.0f);
            rb.wt->translation_.y += step;

            if ((animDir_ < 0 && rb.wt->translation_.y <= targetY) ||
                (animDir_ > 0 && rb.wt->translation_.y >= targetY)) {
                rb.wt->translation_.y = targetY;
            } else {
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

    // ===== 推进 Spike 动画 =====
    bool anySpikeAnimating = false;
    for (auto& st : spikeTiles_) {
        if (!st.animating || !st.wt) continue;
        anySpikeAnimating = true;

        const float targetY = (st.dir < 0) ? st.lowY : st.highY;  // 下落=lowY，上升=highY
        const float step    = moveSpeed_ * ((st.dir < 0) ? -1.0f : 1.0f);
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

    // ===== 动画完成→解锁 =====
    const bool anyAnimatingNow = animating_ || anySpikeAnimating;
    if (playerLocked_ && !anyAnimatingNow) {
        playerLocked_ = false;
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
    for (uint32_t y = 0; y < mapBlocks_.size(); y++) {
        for (uint32_t x = 0; x < mapBlocks_[y].size(); x++) {
            if (mapBlocks_[y][x]) {
                mapBlocks_[y][x]->UpdateMatrix();
                model_->Draw(*mapBlocks_[y][x], camera_);
            }
        }
    }
    for (auto& rb : raisedBlocks_) {
        if (rb.wt) {
            rb.wt->UpdateMatrix();
            obstacleModel_->Draw(*rb.wt, camera_);
        }
    }
    // Spike（仅在显示时绘制 obstacle）
    for (auto& st : spikeTiles_) {
        if (st.wt) {
            st.wt->UpdateMatrix();
            obstacleModel_->Draw(*st.wt, camera_);
        }
    }
    if (player_) player_->Draw();
    Model::PostDraw();

    // 前景スプライト描画
    Sprite::PreDraw(commandList);
    // ここで Portal 上のUIなどを描く場合は WorldToScreen を活用
    Sprite::PostDraw();
}
