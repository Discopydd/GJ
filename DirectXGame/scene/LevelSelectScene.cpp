#include "LevelSelectScene.h"
#include <imgui.h>
#include <dinput.h> // DIK_UP, DIK_DOWN, DIK_RETURN
using namespace KamataEngine;

void LevelSelectScene::Initialize() {
    levels_ = {
        "Resources/map/map.csv",
        "Resources/map/map_a.csv",
        "Resources/map/map_b.csv",
        "Resources/map/map_c.csv",
        "Resources/map/map_d.csv",
        "Resources/map/map_e.csv",
        "Resources/map/map_f.csv",
        "Resources/map/map_g.csv",
    };
    selected_ = 0;
    decided_ = false;
    selectedMap_.clear();
}

void LevelSelectScene::Update() {
    if (decided_ || levels_.empty()) return;

    auto* input = Input::GetInstance();
    const size_t n = levels_.size();

    // ↑/↓ 键选择（避免无符号减法借位）
    if (input->TriggerKey(DIK_UP)) {
        if (selected_ == 0) selected_ = n - 1;
        else selected_ -= 1;
    }
    if (input->TriggerKey(DIK_DOWN)) {
        selected_ = (selected_ + 1) % n;
    }

    // Enter/Space 确认
    if (input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_SPACE)) {
        decided_ = true;
        selectedMap_ = levels_[selected_];
        return;
    }

    // 也支持 ImGui 点击
    ImGui::Begin("Level Select");
    ImGui::Text("Use Up/Down and Enter, or click an item.");
    for (size_t i = 0; i < n; ++i) {
        bool sel = (i == selected_);
        if (ImGui::Selectable(levels_[i].c_str(), sel)) {
            decided_ = true;
            selectedMap_ = levels_[i];
            ImGui::End();
            return;
        }
    }
    ImGui::End();
}

void LevelSelectScene::Draw() {
    // 使用 ImGui 渲染，无需额外 2D/3D 绘制
}
