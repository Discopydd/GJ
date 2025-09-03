#include "Player.h"

Player::~Player()
{
    if (model_) {
        delete model_;
        model_ = nullptr;
    }
}

void Player::Initialize(const Camera* camera, const char* modelName) {
    camera_ = camera;
    wt_.Initialize();
    model_ = Model::CreateFromOBJ(modelName, true);
    if (!model_) {
        model_ = Model::CreateFromOBJ("cube", true);
    }
}

void Player::SetWorldPosition(const Vector3& pos) {
    wt_.translation_ = pos;
}

void Player::SetByTileIndex(const MapChipField& map, uint32_t xIndex, uint32_t yIndex, float yOnTop) {
    Vector3 center2D = map.GetMapChipPositionByIndex(xIndex, yIndex);
    wt_.translation_ = { center2D.x, yOnTop, center2D.y };
}

void Player::Update() {

    wt_.UpdateMatrix();
}

void Player::Draw() {
    if (model_ && camera_) {
        model_->Draw(wt_, *camera_);
    }
}
