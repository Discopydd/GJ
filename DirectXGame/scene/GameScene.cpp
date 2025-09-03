#include "GameScene.h"
#include <cassert>
#include <algorithm>
#include <imgui.h>
using namespace KamataEngine;
using namespace KamataEngine::MathUtility;

Vector2 WorldToScreen(const Vector3& worldPos, const Matrix4x4& view, const Matrix4x4& proj, float windowWidth, float windowHeight) {
    // ワールド→ビュー→射影（w除算込み）
    Matrix4x4 vpMatrix = view * proj;
    Vector3 ndcPos = TransformCoord(worldPos, vpMatrix);  // -1.0 ~ 1.0

    // NDC → スクリーン座標へ変換
    Vector2 screenPos;
    screenPos.x = (ndcPos.x * 0.5f + 0.5f) * windowWidth;
    screenPos.y = (1.0f - (ndcPos.y * 0.5f + 0.5f)) * windowHeight;

    return screenPos;
}


void GameScene::GenerateBlocks() {
	mapBlocks_.clear();
	mapBlocks_.resize(mapChipField_.numBlockVertical_);

	for (uint32_t y = 0; y < mapChipField_.numBlockVertical_; y++) {
		mapBlocks_[y].resize(mapChipField_.numBlockHorizontal_, nullptr);

		for (uint32_t x = 0; x < mapChipField_.numBlockHorizontal_; x++) {
			MapChipType type = mapChipField_.GetMapChipTypeByIndex(x, y);
			if (type == MapChipType::kBlock) {
				auto* wt = new WorldTransform();
                wt->Initialize();

                Vector3 pos2D = mapChipField_.GetMapChipPositionByIndex(x, y);

                float tileHalf = MapChipField::kBlockHeight * 0.5f;

                wt->translation_ = { pos2D.x, tileHalf, pos2D.y };

                mapBlocks_[y][x] = wt;
			}
		}
	}
	model_ = Model::CreateFromOBJ("cube", true);
}

GameScene::GameScene() {}

GameScene::~GameScene() {

}

void GameScene::Initialize() {

	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	camera_.Initialize();
	camera_.translation_ = { -10.0f, 20.0f, -20.0f };
    camera_.rotation_      = { 0.5f, 0.5f, 0.0f };

    camera_.UpdateMatrix();
    mapChipField_.LoadMapChipCsv("Resources/map.csv");

    GenerateBlocks();

}

void GameScene::Update() {

	  ImGui::Begin("Camera Controller");

    static float pos[3];
    static float rot[3];

    // 同步当前值
    pos[0] = camera_.translation_.x;
    pos[1] = camera_.translation_.y;
    pos[2] = camera_.translation_.z;

    rot[0] = camera_.rotation_.x;
    rot[1] = camera_.rotation_.y;
    rot[2] = camera_.rotation_.z;

    // 拖动修改
    if (ImGui::DragFloat3("Position", pos, 0.1f)) {
        camera_.translation_ = { pos[0], pos[1], pos[2] };
    }
    if (ImGui::DragFloat3("Rotation", rot, 0.01f)) {
        camera_.rotation_ = { rot[0], rot[1], rot[2] };
    }

    ImGui::End();
	camera_.UpdateMatrix();
}


void GameScene::Draw() {

	// コマンドリストの取得
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

#pragma region 背景スプライト描画
	// 背景スプライト描画前処理
	Sprite::PreDraw(commandList);

	/// <summary>
	/// ここに背景スプライトの描画処理を追加できる
	/// </summary>
	// スプライト描画後処理
	Sprite::PostDraw();
	// 深度バッファクリア
	dxCommon_->ClearDepthBuffer();
#pragma endregion

#pragma region 3Dオブジェクト描画
	// 3Dオブジェクト描画前処理
	Model::PreDraw();
	for (uint32_t y = 0; y < mapBlocks_.size(); y++) {
		for (uint32_t x = 0; x < mapBlocks_[y].size(); x++) {
			if (mapBlocks_[y][x]) {
				mapBlocks_[y][x]->UpdateMatrix();
				model_->Draw(*mapBlocks_[y][x], camera_);
			}
		}
	}
	/// <summary>
	/// ここに3Dオブジェクトの描画処理を追加できる
	/// </summary>
	// 3Dオブジェクト描画後処理
	Model::PostDraw();
#pragma endregion

#pragma region 前景スプライト描画
	// 前景スプライト描画前処理
	Sprite::PreDraw(commandList);
	/// <summary>
	/// ここに前景スプライトの描画処理を追加できる
	/// </summary>

	// スプライト描画後処理
	Sprite::PostDraw();

#pragma endregion
}

