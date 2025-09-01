#include <Windows.h>
#include"KamataEngine.h"
#include "scene/GameScene.h"
#include "scene/TitleScene.h"

using namespace KamataEngine;



// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	KamataEngine::Initialize(L"LE3C_28_リ_ヨン");
	GameScene* gameScene = nullptr;
	TitleScene* titleScene = nullptr;
	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	titleScene = new TitleScene();
	titleScene->Initialize();

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}
		if (titleScene && !titleScene->IsSceneEnd()) {
			titleScene->Update();
		}
		else {
			if (titleScene) {
				delete titleScene;
				titleScene = nullptr;

				gameScene = new GameScene();
				gameScene->Initialize();
			}

			if (gameScene) {
				gameScene->Update();
			}
		}
		// 描画開始
		dxCommon->PreDraw();
		if (titleScene && !titleScene->IsSceneEnd()) {
			titleScene->Draw();
		}
		else if (gameScene) {
			gameScene->Draw();
		}

		// 描画終了
		dxCommon->PostDraw();
	}
		delete gameScene;
		gameScene = nullptr;
		delete titleScene;
		titleScene = nullptr;
		KamataEngine::Finalize();
	
	return 0;
}
