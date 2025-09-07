#include "Player.h"
#include <cmath>
#include <algorithm>

namespace {
    constexpr float kPI = 3.14159265358979323846f;
    constexpr float kTwoPI = kPI * 2.0f;
}

Player::~Player() {
    if (model_) {
        delete model_;
        model_ = nullptr;
    }
}

void Player::Initialize(const Camera* camera, const MapChipField* map,const char* modelName) {
    camera_ = camera;
    map_ = map;
    wt_.Initialize();
    input_ = Input::GetInstance();

    // モデル読み込み（失敗時はキューブで代用）
    model_ = Model::CreateFromOBJ(modelName, true);
    if (!model_) {
        model_ = Model::CreateFromOBJ("cube", true);
    }

    // 初期朝向 +Z
    currentYaw_ = kPI * 0.5f;
    startYaw_ = targetYaw_ = currentYaw_;
    wt_.rotation_.y = currentYaw_;
}

// ワールド座標設定
void Player::SetWorldPosition(const Vector3& pos) {
    wt_.translation_ = pos;
}

// タイル座標からワールド座標に変換して設定
void Player::SetByTileIndex(const MapChipField& map, uint32_t xIndex, uint32_t yIndex, float yOnTop) {
    ix_ = xIndex;  iy_ = yIndex;
    Vector3 c = map.GetMapChipPositionByIndex(ix_, iy_);
    wt_.translation_ = { c.x, yOnTop, c.y };
}

// ====== 回転関連 ======
// 角度を(-PI, PI]に正規化
float Player::NormalizeAngle(float a) {
    while (a <= -kPI) a += kTwoPI;
    while (a >   kPI) a -= kTwoPI;
    return a;
}

// 最短の角度差を計算
float Player::ShortestDelta(float from, float to) {
    return NormalizeAngle(to - from);
}

// 特定の角度へ回転リクエスト
void Player::RequestFaceYaw(float yaw) {
    float want = NormalizeAngle(yaw);
    float delta = ShortestDelta(currentYaw_, want);
    if (std::fabs(delta) < 0.001f) {
        isRotating_ = false;
        startYaw_ = targetYaw_ = currentYaw_ = want;
        return;
    }
    startYaw_ = currentYaw_;
    targetYaw_ = currentYaw_ + delta;
    rotateT_ = 0.0f;
    isRotating_ = true;
}

// 毎フレーム回転更新
void Player::UpdateRotation(float dt) {
    if (!isRotating_) return;
    float dur = (std::max)(RotateDuration, 0.0001f);
    rotateT_ += dt / dur;
    if (rotateT_ >= 1.0f) {
        rotateT_ = 1.0f;
        currentYaw_ = NormalizeAngle(targetYaw_);
        isRotating_ = false;
    } else {
        float t = rotateT_;
        t = t * t * (3.0f - 2.0f * t);   // イージング
        currentYaw_ = NormalizeAngle(startYaw_ + (targetYaw_ - startYaw_) * t);
    }
    wt_.rotation_.y = currentYaw_;
}

// 毎フレーム更新
void Player::Update() {
    if (!map_) return;

    const float dt = 1.0f / 60.0f; // 固定タイムステップ

    // A) 回転入力処理
    bool hasYaw = false;
    float wantedYaw = 0.0f;
    if (input_->TriggerKey(DIK_RIGHT) || input_->TriggerKey(DIK_D)) { wantedYaw = 0.0f;        hasYaw = true; }
    if (input_->TriggerKey(DIK_LEFT)  || input_->TriggerKey(DIK_A)) { wantedYaw = kPI;         hasYaw = true; }
    if (input_->TriggerKey(DIK_UP)    || input_->TriggerKey(DIK_W)) { wantedYaw = -kPI * 0.5f; hasYaw = true; }
    if (input_->TriggerKey(DIK_DOWN)  || input_->TriggerKey(DIK_S)) { wantedYaw = kPI * 0.5f;  hasYaw = true; }
    if (hasYaw) {
        RequestFaceYaw(wantedYaw);
    }
    UpdateRotation(dt); // 回転進行

    const uint32_t W = map_->numBlockHorizontal_;
    const uint32_t H = map_->numBlockVertical_;
    const float blockTopY = MapChipField::kBlockHeight;
    const float playerCenterY = blockTopY + height_ * 0.5f;

    // B) 移動中なら補間
    if (isMoving_) {
        moveT_ += (MoveDuration > 0.f ? dt / MoveDuration : 1.f);
        if (moveT_ >= 1.0f) {
            moveT_ = 1.0f;
            wt_.translation_ = targetPos_;
            isMoving_ = false;
        } else {
            float t = moveT_;
            t = t * t * (3.0f - 2.0f * t);
            wt_.translation_ = {
                startPos_.x + (targetPos_.x - startPos_.x) * t,
                startPos_.y + (targetPos_.y - startPos_.y) * t,
                startPos_.z + (targetPos_.z - startPos_.z) * t
            };
        }
        wt_.UpdateMatrix();
        return;
    }

    // C) 未移動 → 入力で次のマスを決定
    int nx = static_cast<int>(ix_);
    int ny = static_cast<int>(iy_);

    if (input_->TriggerKey(DIK_RIGHT) || input_->TriggerKey(DIK_D)) { nx += 1; }
    if (input_->TriggerKey(DIK_LEFT)  || input_->TriggerKey(DIK_A)) { nx -= 1; }
    if (input_->TriggerKey(DIK_UP)    || input_->TriggerKey(DIK_W)) { ny += 1; }
    if (input_->TriggerKey(DIK_DOWN)  || input_->TriggerKey(DIK_S)) { ny -= 1; }

    // 入力なし
    if (nx == static_cast<int>(ix_) && ny == static_cast<int>(iy_)) {
        wt_.UpdateMatrix();
        return;
    }

    // 境界チェック
    if (nx < 0 || ny < 0 || nx >= static_cast<int>(W) || ny >= static_cast<int>(H)) {
        wt_.UpdateMatrix();
        return;
    }
    // 壁・Raised禁止
    {
        MapChipType t = map_->GetMapChipTypeByIndex((uint32_t)nx, (uint32_t)ny);
        if (t == MapChipType::kBlank || t == MapChipType::kRaised|| t == MapChipType::kSpike) {
            wt_.UpdateMatrix();
            return;
        }
    }
    // ターゲット位置計算
    Vector3 c = map_->GetMapChipPositionByIndex(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny));
    Vector3 nextCenter = { c.x, playerCenterY, c.y };

    // 移動開始
    ix_ = (uint32_t)nx;
    iy_ = (uint32_t)ny;
    startPos_  = wt_.translation_;
    targetPos_ = nextCenter;
    moveT_     = 0.0f;
    isMoving_  = true;

    wt_.UpdateMatrix();
}

// 描画
void Player::Draw() {
    if (model_ && camera_) {
        model_->Draw(wt_, *camera_);
    }
}

void Player::ResetOrientation()
{
    currentYaw_ = startYaw_ = targetYaw_ = kPI * 0.5f; // +Z 方向
    isRotating_ = false;
    rotateT_ = 0.0f;
    wt_.rotation_.y = currentYaw_;
}
