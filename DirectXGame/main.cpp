#include <Windows.h>
#include "KamataEngine.h"
#include "scene/GameScene.h"
#include "scene/TitleScene.h"

using namespace KamataEngine;

// 辅助：应用全屏/窗口
static void ApplyFullscreen(HWND hwnd, bool fullscreen, DWORD windowedStyle, const RECT& windowedRect) {
    if (fullscreen) {
        // 取最近显示器的工作区（含任务栏用 rcMonitor，纯显示区也可用 rcMonitor）
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);

        // 切到无边框窗口化全屏
        SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_TOP,
                     mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_FRAMECHANGED);
    } else {
        // 恢复窗口样式与原窗口矩形
        SetWindowLong(hwnd, GWL_STYLE, windowedStyle | WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_NOTOPMOST,
                     windowedRect.left, windowedRect.top,
                     windowedRect.right - windowedRect.left,
                     windowedRect.bottom - windowedRect.top,
                     SWP_FRAMECHANGED);
    }
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    KamataEngine::Initialize(L"異界ダンジョン");

    // ====== ESC 切换 全屏/窗口 的初始化 ======
    HWND hwnd = WinApp::GetInstance()->GetHwnd();
    // 记录当前窗口样式与窗口矩形（用于恢复）
    DWORD windowedStyle = GetWindowLong(hwnd, GWL_STYLE);
    RECT  windowedRect{};
    if (!GetWindowRect(hwnd, &windowedRect)) {
        // 若获取失败，给一个合理默认窗口位置与大小
        windowedRect = { 100, 100, 100 + 1280, 100 + 720 };
    }
    bool isFullscreen = false; // 若想启动即为全屏，把它设为 true 并调用 ApplyFullscreen(hwnd, true, ...)

    GameScene* gameScene = nullptr;
    TitleScene* titleScene = nullptr;

    // DirectXCommonインスタンスの取得
    DirectXCommon* dxCommon = DirectXCommon::GetInstance();
    ImGuiManager* imguiManager = ImGuiManager::GetInstance();

    titleScene = new TitleScene();
    titleScene->Initialize();

    while (true) {
        if (KamataEngine::Update()) {
            break;
        }

        // ====== 监听 ESC：按一次切换 ======
        if (GetAsyncKeyState(VK_ESCAPE) & 0x1) { // 0x1 = 只在按下瞬间触发
            isFullscreen = !isFullscreen;

            // 切换前若当前是窗口模式，先更新一下最新的窗口矩形，保证恢复时准确
            if (!isFullscreen) {
                // no-op
            } else {
                // 进入全屏前记录窗口最新位置（可选：只在从窗口 -> 全屏时更新）
                GetWindowRect(hwnd, (LPRECT)&windowedRect);
                windowedStyle = GetWindowLong(hwnd, GWL_STYLE);
            }

            ApplyFullscreen(hwnd, isFullscreen, windowedStyle, windowedRect);
        }

        imguiManager->Begin();

        if (titleScene && !titleScene->IsSceneEnd()) {
            titleScene->Update();
        } else {
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
        imguiManager->End();

        // 描画開始
        dxCommon->PreDraw();
        if (titleScene && !titleScene->IsSceneEnd()) {
            titleScene->Draw();
        } else if (gameScene) {
            gameScene->Draw();
        }
        imguiManager->Draw();

        // 描画終了
        dxCommon->PostDraw();
    }

    delete gameScene;  gameScene = nullptr;
    delete titleScene; titleScene = nullptr;
    KamataEngine::Finalize();
    return 0;
}
