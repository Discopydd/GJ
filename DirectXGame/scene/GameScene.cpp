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
	raisedBlocks_.clear();
	for (uint32_t y = 0; y < mapChipField_.numBlockVertical_; y++) {
		mapBlocks_[y].resize(mapChipField_.numBlockHorizontal_, nullptr);

		for (uint32_t x = 0; x < mapChipField_.numBlockHorizontal_; x++) {
			MapChipType type = mapChipField_.GetMapChipTypeByIndex(x, y);
			if (type == MapChipType::kBlock|| type == MapChipType::kPortal || type == MapChipType::kRaised) {
				auto* wt = new WorldTransform();
                wt->Initialize();

                Vector3 pos2D = mapChipField_.GetMapChipPositionByIndex(x, y);

                float tileHalf = MapChipField::kBlockHeight * 0.5f;

                wt->translation_ = { pos2D.x, tileHalf, pos2D.y };

                mapBlocks_[y][x] = wt;
				if (type == MapChipType::kRaised) {
                    auto* raised = new WorldTransform();
                    raised->Initialize();
                    // 悬起一层：在 Y 再加一个方块高度
                    raised->translation_ = { pos2D.x, tileHalf + MapChipField::kBlockHeight, pos2D.y };
                    raisedBlocks_.push_back(RaisedBlock{ raised, x, y });
                }
			}
		}
	}
	model_ = Model::CreateFromOBJ("cube", true);
}

GameScene::GameScene() {}

GameScene::~GameScene() {
	delete player_;
}

void GameScene::Initialize() {

	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	camera_.Initialize();
	camera_.translation_ = { -10.0f, 20.0f, -20.0f };
	camera_.rotation_ = { 0.5f, 0.5f, 0.0f };

	camera_.UpdateMatrix();
	mapChipField_.LoadMapChipCsv("Resources/map.csv");

	GenerateBlocks();

	player_ = new Player();
	player_->Initialize(&camera_,&mapChipField_, "player");

	uint32_t topY = (mapChipField_.numBlockVertical_ > 0) ? (mapChipField_.numBlockVertical_ - 1) : 0;
	float blockTopY = MapChipField::kBlockHeight;
	float playerCenterY = blockTopY + player_->GetHeight() * 0.5f;
	player_->SetByTileIndex(mapChipField_, 0, topY, playerCenterY);
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
	if (dropTriggered_) {
		// 方块中心落到地面时的Y（与你项目的格子高度保持一致）
		const float groundCenterY = MapChipField::kBlockHeight * 0.5f;
		const float eps = 1e-4f;

		// 注意：手动递增迭代器，便于在落地后 erase
		for (auto it = raisedBlocks_.begin(); it != raisedBlocks_.end(); /* 手动递增 */) {
			auto& rb = *it; // rb: { WorldTransform* wt; uint32_t x, y; }

			// — 下落 —
			if (rb.wt) {
				rb.wt->translation_.y -= dropSpeed_;   // dropSpeed_ 是你类里已有的速度（每帧下降量）
				if (rb.wt->translation_.y <= groundCenterY + eps) {
					// 到地：对齐
					rb.wt->translation_.y = groundCenterY;

					// ★ 关键：同步“逻辑地图”类型（把 kRaised 改成你需要的真实类型）
					// 方案A：落地后仍阻挡
					mapChipField_.SetMapChipTypeByIndex(rb.x, rb.y, MapChipType::kBlock);

					// —— 若你的设计是“落地后让路”，用下面这一行替换上一行 —— 
					// mapChipField_.SetMapChipTypeByIndex(rb.x, rb.y, MapChipType::kBlank);

					// （可选）并入静态方块渲染列表（如果你有 mapBlocks_ 之类的容器）
					// 例如：CreateCubeAt(mapChipField_.GetMapChipPositionByIndex(rb.x, rb.y));

					// 从“下落列表”移除该块
					it = raisedBlocks_.erase(it);
					continue;
				}
			}

			++it;
		}

		// （可选）当所有悬起块都已落地时，重置触发标记
		if (raisedBlocks_.empty()) {
			dropTriggered_ = false;
		}
	}
	if (player_) player_->Update();
	if (player_) {
		uint32_t px = player_->TileX();
		uint32_t py = player_->TileY();
		MapChipType t = mapChipField_.GetMapChipTypeByIndex(px, py);
		if (!dropTriggered_ && t == MapChipType::kPortal) {
			dropTriggered_ = true;
		}
	}
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
	for (auto& rb : raisedBlocks_) {
		if (rb.wt) {
			rb.wt->UpdateMatrix();
			model_->Draw(*rb.wt, camera_);
		}
	}
	if (player_) player_->Draw();
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

