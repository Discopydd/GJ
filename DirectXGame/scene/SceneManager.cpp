#include "SceneManager.h"

SceneManager::~SceneManager() {
    if (scene_) {
        scene_->Finalize();
        delete scene_;
    }
    if (nextScene_) {
        delete nextScene_;
    }
}

void SceneManager::SetNextScene(IScene* scene) {
    nextScene_ = scene;
}

void SceneManager::Update() {
    // 切换场景
    if (nextScene_) {
        if (scene_) {
            scene_->Finalize();
            delete scene_;
        }
        scene_ = nextScene_;
        nextScene_ = nullptr;
        scene_->Initialize();
    }
    if (scene_) {
        scene_->Update();
    }
}

void SceneManager::Draw() {
    if (scene_) {
        scene_->Draw();
    }
}
