#include "Player.h"

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
}

void Player::SetWorldPosition(const Vector3& pos) {
    wt_.translation_ = pos;
}

void Player::SetByTileIndex(const MapChipField& map, uint32_t xIndex, uint32_t yIndex, float yOnTop) {
    ix_ = xIndex;  iy_ = yIndex;
    Vector3 c = map.GetMapChipPositionByIndex(ix_, iy_); // 返回格中心 (X,Z)
    wt_.translation_ = { c.x, yOnTop, c.y };
}

void Player::Update() {
    if (!map_) return;

    Input* input = Input::GetInstance();

    const uint32_t W = map_->numBlockHorizontal_;
    const uint32_t H = map_->numBlockVertical_;
    const float blockTopY = MapChipField::kBlockHeight;
    const float playerCenterY = blockTopY + height_ * 0.5f;

    // === 1) 若正在移动：插值到目标 ===
    if (isMoving_) {
        // 用你项目里的 deltaTime，如果没有可用固定步长：例如 1/60.f
        const float dt = 1.0f / 60.0f;
        moveT_ += (MoveDuration > 0.f ? dt / MoveDuration : 1.f);
        if (moveT_ >= 1.0f) {
            moveT_ = 1.0f;
            wt_.translation_ = targetPos_;   // 对齐终点
            isMoving_ = false;               // 完成
        } else {
            // 线性 or 平滑曲线（可切换）
            float t = moveT_;
            // 平滑些可用 easeInOut：t = t*t*(3-2*t);
            t = t * t * (3.0f - 2.0f * t);
            wt_.translation_ = {
                startPos_.x + (targetPos_.x - startPos_.x) * t,
                startPos_.y + (targetPos_.y - startPos_.y) * t,
                startPos_.z + (targetPos_.z - startPos_.z) * t
            };
        }
        wt_.UpdateMatrix();
        return; // 动画中不接收新输入
    }

    // === 2) 未在移动：读取一次性输入，决定下一个格子 ===
    int nx = static_cast<int>(ix_);
    int ny = static_cast<int>(iy_);

    if (input->TriggerKey(DIK_RIGHT) || input->TriggerKey(DIK_D)) { nx += 1; }
    if (input->TriggerKey(DIK_LEFT)  || input->TriggerKey(DIK_A)) { nx -= 1; }
    if (input->TriggerKey(DIK_UP)    || input->TriggerKey(DIK_W)) { ny += 1; } // 若方向相反就把+/-对调
    if (input->TriggerKey(DIK_DOWN)  || input->TriggerKey(DIK_S)) { ny -= 1; }

    // 没有输入就直接更新矩阵并返回
    if (nx == static_cast<int>(ix_) && ny == static_cast<int>(iy_)) {
        wt_.UpdateMatrix();
        return;
    }

    // === 3) 边界检查（必要时也可在这里加“不能走进墙块”的判定） ===
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
