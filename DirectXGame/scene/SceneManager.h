#pragma once
#include "IScene.h"

class SceneManager {
public:
    ~SceneManager();

    void SetNextScene(IScene* scene);
    void Update();
    void Draw();

private:
    IScene* scene_ = nullptr;
    IScene* nextScene_ = nullptr;
};
