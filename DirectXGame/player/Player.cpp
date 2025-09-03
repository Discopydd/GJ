#include "Player.h"
#include <cmath>
#include <algorithm>

namespace {
    constexpr float kPI = 3.14159265358979323846f;
    constexpr float kTwoPI = kPI * 2.0f;
}

Player::~Player()
{
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
    model_ = Model::CreateFromOBJ(modelName, true);
    if (!model_) {
        model_ = Model::CreateFromOBJ("cube", true);
    }

    // 初始朝向 +X
    currentYaw_ = 0.0f;
    startYaw_ = targetYaw_ = currentYaw_;
    wt_.rotation_.y = currentYaw_;
}

void Player::SetWorldPosition(const Vector3& pos) {
    wt_.translation_ = pos;
}

void Player::SetByTileIndex(const MapChipField& map, uint32_t xIndex, uint32_t yIndex, float yOnTop) {
    ix_ = xIndex;  iy_ = yIndex;
    Vector3 c = map.GetMapChipPositionByIndex(ix_, iy_); // 返回格中心 (X,Z)
    wt_.translation_ = { c.x, yOnTop, c.y };
}

// ====== 旋转相关 ======
float Player::NormalizeAngle(float a) {
    // 归一化到 (-PI, PI]
    while (a <= -kPI) a += kTwoPI;
    while (a >   kPI) a -= kTwoPI;
    return a;
}

float Player::ShortestDelta(float from, float to) {
    return NormalizeAngle(to - from);
}

void Player::RequestFaceYaw(float yaw) {
    float want = NormalizeAngle(yaw);
    float delta = ShortestDelta(currentYaw_, want);
    // 足够接近就不转
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
        // easeInOut
        t = t * t * (3.0f - 2.0f * t);
        currentYaw_ = NormalizeAngle(startYaw_ + (targetYaw_ - startYaw_) * t);
    }
    wt_.rotation_.y = currentYaw_;
}

void Player::Update() {
    if (!map_) return;

    // 固定步长（若你有 deltaTime，请改成实际 dt）
    const float dt = 1.0f / 60.0f;

    Input* input = Input::GetInstance();

    // ===== A) 处理朝向输入（可与移动并行） =====
    // 约定按键方向与朝向：右=+X, 左=-X, 上=+Z, 下=-Z
    bool hasYaw = false;
    float wantedYaw = 0.0f;
    if (input->TriggerKey(DIK_RIGHT) || input->TriggerKey(DIK_D)) { wantedYaw = 0.0f;          hasYaw = true; }
    if (input->TriggerKey(DIK_LEFT)  || input->TriggerKey(DIK_A)) { wantedYaw = kPI;           hasYaw = true; }
    if (input->TriggerKey(DIK_UP)    || input->TriggerKey(DIK_W)) { wantedYaw = -kPI * 0.5f;    hasYaw = true; }
    if (input->TriggerKey(DIK_DOWN)  || input->TriggerKey(DIK_S)) { wantedYaw = kPI * 0.5f;   hasYaw = true; }
    if (hasYaw) {
        RequestFaceYaw(wantedYaw);
    }
    // 推进旋转（无论是否在移动，都要平滑转向）
    UpdateRotation(dt);

    const uint32_t W = map_->numBlockHorizontal_;
    const uint32_t H = map_->numBlockVertical_;
    const float blockTopY = MapChipField::kBlockHeight;
    const float playerCenterY = blockTopY + height_ * 0.5f;

    // ===== B) 若正在移动：插值到目标 =====
    if (isMoving_) {
        moveT_ += (MoveDuration > 0.f ? dt / MoveDuration : 1.f);
        if (moveT_ >= 1.0f) {
            moveT_ = 1.0f;
            wt_.translation_ = targetPos_;   // 对齐终点
            isMoving_ = false;               // 完成
        } else {
            float t = moveT_;
            t = t * t * (3.0f - 2.0f * t);   // easeInOut
            wt_.translation_ = {
                startPos_.x + (targetPos_.x - startPos_.x) * t,
                startPos_.y + (targetPos_.y - startPos_.y) * t,
                startPos_.z + (targetPos_.z - startPos_.z) * t
            };
        }
        wt_.UpdateMatrix(); // 包含当前旋转
        return;             // 动画中不接收新的移动输入
    }

    // ===== C) 未在移动：读取一次性输入，决定下一个格子 =====
    int nx = static_cast<int>(ix_);
    int ny = static_cast<int>(iy_);

    if (input->TriggerKey(DIK_RIGHT) || input->TriggerKey(DIK_D)) { nx += 1; }
    if (input->TriggerKey(DIK_LEFT)  || input->TriggerKey(DIK_A)) { nx -= 1; }
    if (input->TriggerKey(DIK_UP)    || input->TriggerKey(DIK_W)) { ny += 1; } // 若方向相反可对调+/-
    if (input->TriggerKey(DIK_DOWN)  || input->TriggerKey(DIK_S)) { ny -= 1; }

    // 没有移动输入 → 只更新矩阵（旋转已在上面处理）
    if (nx == static_cast<int>(ix_) && ny == static_cast<int>(iy_)) {
        wt_.UpdateMatrix();
        return;
    }

    // 边界检查（如需阻止进墙，可在此加判断）
    if (nx < 0 || ny < 0 || nx >= static_cast<int>(W) || ny >= static_cast<int>(H)) {
        wt_.UpdateMatrix();
        return; // 超出边界：忽略输入
    }
    // 阻止走进墙：
    // if (map_->GetMapChipTypeByIndex(nx, ny) == MapChipType::kBlock) { wt_.UpdateMatrix(); return; }

    // 计算目标世界坐标（格中心 XZ + 顶面高度上的玩家中心 Y）
    Vector3 c = map_->GetMapChipPositionByIndex(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny));
    Vector3 nextCenter = { c.x, playerCenterY, c.y };

    // 设置移动状态
    ix_ = static_cast<uint32_t>(nx);
    iy_ = static_cast<uint32_t>(ny);
    startPos_  = wt_.translation_;
    targetPos_ = nextCenter;
    moveT_     = 0.0f;
    isMoving_  = true;

    wt_.UpdateMatrix();
}

void Player::Draw() {
    if (model_ && camera_) {
        model_->Draw(wt_, *camera_);
    }
}
