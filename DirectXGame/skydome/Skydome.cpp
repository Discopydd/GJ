#include "Skydome.h"
using namespace KamataEngine;
Skydome::~Skydome() { delete model_; }

void Skydome::Initialize(Camera* camera, const char* modelName) {
    camera_ = camera;
    worldTransform_.Initialize();
    model_ = Model::CreateFromOBJ(modelName, true);
    worldTransform_.scale_ = { 100, 100, 100 };
}

void Skydome::Update() {
    worldTransform_.UpdateMatrix();
}

void Skydome::Draw() {
    if (model_ && camera_) {
        model_->Draw(worldTransform_, *camera_);
    }
}
