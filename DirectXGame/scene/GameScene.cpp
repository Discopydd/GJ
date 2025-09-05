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

// マップからブロックを生成
void GameScene::GenerateBlocks() {
    // 以前の生成物を破棄（リーク防止）
    DisposeMapBlocks(mapBlocks_);
    DisposeRaised(raisedBlocks_);

    mapBlocks_.resize(mapChipField_.numBlockVertical_);

    for (uint32_t y = 0; y < mapChipField_.numBlockVertical_; y++) {
        mapBlocks_[y].resize(mapChipField_.numBlockHorizontal_, nullptr);

        for (uint32_t x = 0; x < mapChipField_.numBlockHorizontal_; x++) {
            MapChipType type = mapChipField_.GetMapChipTypeByIndex(x, y);
            if (type == MapChipType::kBlock || type == MapChipType::kPortal || type == MapChipType::kRaised) {
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
                    float lowY  = tileHalf;                               // 地面の高さ

                    raised->translation_ = { pos2D.x, highY, pos2D.y };
                    raisedBlocks_.push_back(RaisedBlock{ raised, x, y, highY, lowY });

                    // 初期は「上に居る」状態（＝上昇完了）とみなす
                    isLowered_ = false;
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
    // カメラ調整用UI
    ImGui::Begin("Camera Controller");
    static float pos[3];
    static float rot[3];
    pos[0] = camera_.translation_.x; pos[1] = camera_.translation_.y; pos[2] = camera_.translation_.z;
    rot[0] = camera_.rotation_.x;    rot[1] = camera_.rotation_.y;    rot[2] = camera_.rotation_.z;
    if (ImGui::DragFloat3("Position", pos, 0.1f)) { camera_.translation_ = { pos[0], pos[1], pos[2] }; }
    if (ImGui::DragFloat3("Rotation", rot, 0.01f)) { camera_.rotation_    = { rot[0], rot[1], rot[2] }; }
    ImGui::End();
    camera_.UpdateMatrix();

    // ---- (A) Raised アニメーション ----
    if (animating_) {
        bool allDone = true;
        for (auto& rb : raisedBlocks_) {
            if (!rb.wt) continue;

            const float targetY = (animDir_ < 0) ? rb.lowY : rb.highY;
            const float step    = moveSpeed_ * ((animDir_ < 0) ? -1.0f : 1.0f);

            rb.wt->translation_.y += step;

            // 目標到達のクランプ
            if ((animDir_ < 0 && rb.wt->translation_.y <= targetY) ||
                (animDir_ > 0 && rb.wt->translation_.y >= targetY)) {
                rb.wt->translation_.y = targetY;
            } else {
                allDone = false;
            }
        }

        if (allDone) {
            animating_ = false;
            isLowered_ = (animDir_ < 0); // 今回の到達状態を記録

            // マップデータへ同期（落下完了→Block / 上昇完了→Raised）
            for (auto& rb : raisedBlocks_) {
                mapChipField_.SetMapChipTypeByIndex(
                    rb.x, rb.y,
                    isLowered_ ? MapChipType::kBlock : MapChipType::kRaised
                );
            }
        }
    }

    // ---- (B) プレイヤー更新 ----
    if (player_) player_->Update();

    // ---- (C) Portal 辺縁トリガー ----
    bool onPortal = false;
    if (player_) {
        uint32_t px = player_->TileX();
        uint32_t py = player_->TileY();
        MapChipType t = mapChipField_.GetMapChipTypeByIndex(px, py);
        onPortal = (t == MapChipType::kPortal);
    }

    // 「前フレームは不在」→「今フレームは在」の瞬間かつ非アニメ中で Raised があるならトグル
    if (onPortal && !wasOnPortal_ && !animating_ && !raisedBlocks_.empty()) {
        animating_ = true;
        animDir_   = isLowered_ ? +1 : -1; // 落ちてたら上げる、上に居たら落とす
    }
    wasOnPortal_ = onPortal;
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
    if (player_) player_->Draw();
    Model::PostDraw();

    // 前景スプライト描画
    Sprite::PreDraw(commandList);
    // ここで Portal 上のUIなどを描く場合は WorldToScreen を活用
    Sprite::PostDraw();
}
