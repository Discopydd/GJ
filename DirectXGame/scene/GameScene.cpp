#include "GameScene.h"
#include <cassert>
#include <algorithm>
#include <imgui.h>
using namespace KamataEngine;
using namespace KamataEngine::MathUtility;

// ワールド座標をスクリーン座標に変換
Vector2 WorldToScreen(const Vector3& worldPos, const Matrix4x4& view, const Matrix4x4& proj, float windowWidth, float windowHeight) {
    Matrix4x4 vpMatrix = view * proj;
    Vector3 ndcPos = TransformCoord(worldPos, vpMatrix);
    Vector2 screenPos;
    screenPos.x = (ndcPos.x * 0.5f + 0.5f) * windowWidth;
    screenPos.y = (1.0f - (ndcPos.y * 0.5f + 0.5f)) * windowHeight;
    return screenPos;
}

// マップからブロックを生成
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
                    // RaisedはYを1段上に配置
                    raised->translation_ = { pos2D.x, tileHalf + MapChipField::kBlockHeight, pos2D.y };
                    raisedBlocks_.push_back(RaisedBlock{ raised, x, y });
                }
			}
		}
	}
	model_ = Model::CreateFromOBJ("cube", true);
}

GameScene::GameScene() {}
GameScene::~GameScene() { delete player_; }

// 初期化
void GameScene::Initialize() {
	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	camera_.Initialize();
	camera_.translation_ = { -10.0f, 20.0f, -20.0f };
	camera_.rotation_ = { 0.5f, 0.5f, 0.0f };
	camera_.UpdateMatrix();

	// マップ読み込み
	mapChipField_.LoadMapChipCsv("Resources/map.csv");
	GenerateBlocks();

	// プレイヤー初期化
	player_ = new Player();
	player_->Initialize(&camera_,&mapChipField_, "player");

	uint32_t topY = (mapChipField_.numBlockVertical_ > 0) ? (mapChipField_.numBlockVertical_ - 1) : 0;
	float blockTopY = MapChipField::kBlockHeight;
	float playerCenterY = blockTopY + player_->GetHeight() * 0.5f;
	player_->SetByTileIndex(mapChipField_, 0, topY, playerCenterY);
}

// 毎フレーム更新
void GameScene::Update() {
	// カメラ調整用UI
	ImGui::Begin("Camera Controller");
	static float pos[3];
	static float rot[3];
	pos[0] = camera_.translation_.x; pos[1] = camera_.translation_.y; pos[2] = camera_.translation_.z;
	rot[0] = camera_.rotation_.x; rot[1] = camera_.rotation_.y; rot[2] = camera_.rotation_.z;
	if (ImGui::DragFloat3("Position", pos, 0.1f)) { camera_.translation_ = { pos[0], pos[1], pos[2] }; }
	if (ImGui::DragFloat3("Rotation", rot, 0.01f)) { camera_.rotation_ = { rot[0], rot[1], rot[2] }; }
	ImGui::End();
	camera_.UpdateMatrix();

	// Raisedブロック落下処理
	if (dropTriggered_) {
		const float groundCenterY = MapChipField::kBlockHeight * 0.5f;
		const float eps = 1e-4f;
		for (auto it = raisedBlocks_.begin(); it != raisedBlocks_.end();) {
			auto& rb = *it;
			if (rb.wt) {
				rb.wt->translation_.y -= dropSpeed_;
				if (rb.wt->translation_.y <= groundCenterY + eps) {
					rb.wt->translation_.y = groundCenterY;
					// 落下完了後マップデータ更新（阻止する場合はkBlock / 通行可にするならkBlank）
					mapChipField_.SetMapChipTypeByIndex(rb.x, rb.y, MapChipType::kBlock);
					it = raisedBlocks_.erase(it);
					continue;
				}
			}
			++it;
		}
		if (raisedBlocks_.empty()) { dropTriggered_ = false; }
	}

	// プレイヤー更新
	if (player_) player_->Update();

	// ポータル判定 → 落下開始
	if (player_) {
		uint32_t px = player_->TileX();
		uint32_t py = player_->TileY();
		MapChipType t = mapChipField_.GetMapChipTypeByIndex(px, py);
		if (!dropTriggered_ && t == MapChipType::kPortal) {
			dropTriggered_ = true;
		}
	}
}

// 描画
void GameScene::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	// 背景スプライト描画
	Sprite::PreDraw(commandList);


	Sprite::PostDraw();
	dxCommon_->ClearDepthBuffer();

	// 3Dオブジェクト描画
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
	Model::PostDraw();

	// 前景スプライト描画
	Sprite::PreDraw(commandList);


	Sprite::PostDraw();
}
