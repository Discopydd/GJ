#pragma once
#include <vector>
#include <string>
#include "KamataEngine.h"

class LevelSelectScene {
public:
    void Initialize();
    void Update();
    void Draw();
    void Finalize() {}

    // main.cpp 会用到
    bool IsSceneEnd() const { return decided_; }
    std::string GetSelectedMap() const { return selectedMap_; }

private:
    std::vector<std::string> levels_;
    size_t selected_ = 0;       // 用 size_t 避免 C4267
    bool decided_ = false;      // 选关确认
    std::string selectedMap_;   // 确认后的地图路径
};
