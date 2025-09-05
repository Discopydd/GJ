#pragma once
#include"KamataEngine.h"
#include "../map/MapChipField.h"
#include "../player/Player.h" 
using namespace KamataEngine;

/// <summary>
/// ゲームシーン管理クラス
/// </summary>
class GameScene {

public:
	GameScene();   // コンストラクタ
	~GameScene();  // デストラクタ

	void Initialize(); // 初期化
	void Update();     // 毎フレーム更新
	void Draw();       // 描画

private:
	DirectXCommon* dxCommon_ = nullptr;
	Input* input_ = nullptr;
	Camera camera_;
	KamataEngine::Model* model_ = nullptr;

	MapChipField mapChipField_;   // マップチップデータ
	std::vector<std::vector<WorldTransform*>> mapBlocks_; // 生成されたブロック
	void GenerateBlocks(); // ブロック生成

	Player* player_ = nullptr;

	// Raised（浮いているブロック）
	struct RaisedBlock {
		WorldTransform* wt = nullptr;
		uint32_t x = 0, y = 0;
	};
	std::vector<RaisedBlock> raisedBlocks_;
	bool dropTriggered_ = false;   // 落下開始フラグ
	float dropSpeed_ = 0.25f;      // 落下速度
};
