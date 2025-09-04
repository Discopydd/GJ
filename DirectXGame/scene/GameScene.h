#pragma once
#include"KamataEngine.h"
#include "../map/MapChipField.h"
#include "../player/Player.h" 
using namespace KamataEngine;
/// <summary>
/// ゲームシーン
/// </summary>
class GameScene {

public: // メンバ関数
	/// <summary>
	/// コンストクラタ
	/// </summary>
	GameScene();

	/// <summary>
	/// デストラクタ
	/// </summary>
	~GameScene();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 毎フレーム処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();
private: // メンバ変数
	DirectXCommon* dxCommon_ = nullptr;
	Input* input_ = nullptr;
	Camera camera_;
	KamataEngine::Model* model_ = nullptr;

	MapChipField mapChipField_;   // 地图数据
	std::vector<std::vector<WorldTransform*>> mapBlocks_; // 存放生成的方块对象
	void GenerateBlocks();

	Player* player_ = nullptr;

	struct RaisedBlock {
		WorldTransform* wt = nullptr;
		uint32_t x = 0, y = 0;
	};
	std::vector<RaisedBlock> raisedBlocks_;
	bool dropTriggered_ = false;   // 是否已触发“下落”
	float dropSpeed_ = 0.25f;      // 每帧下降的距离（可调；按 60fps 约 0.25 * 60 = 15u/s）



	/// <summary>
	/// ゲームシーン用
	/// </summary>
};