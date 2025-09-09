#include <Windows.h>
#include "KamataEngine.h"
#include "scene/SceneManager.h"
#include "scene/TitleScene.h"

using namespace KamataEngine;

// 辅助：应用全屏/窗口（沿用你现有实现）
static void ApplyFullscreen(HWND hwnd, bool fullscreen, DWORD windowedStyle, const RECT& windowedRect) {
    if (fullscreen) {
        MONITORINFO mi{}; mi.cbSize = sizeof(mi);
        GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_TOP,
                     mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_FRAMECHANGED);
    } else {
        SetWindowLong(hwnd, GWL_STYLE, windowedStyle | WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_NOTOPMOST,
                     windowedRect.left, windowedRect.top,
                     windowedRect.right - windowedRect.left,
                     windowedRect.bottom - windowedRect.top,
                     SWP_FRAMECHANGED);
    }
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    KamataEngine::Initialize(L"異界ダンジョン");

    // ====== ESC 切换 全屏/窗口 的初始化（沿用你现有代码）======
    HWND hwnd = WinApp::GetInstance()->GetHwnd();
    DWORD windowedStyle = GetWindowLong(hwnd, GWL_STYLE);
    RECT  windowedRect{};
    if (!GetWindowRect(hwnd, &windowedRect)) { windowedRect = {100,100,100+1280,100+720}; }
    bool isFullscreen = false;

    auto* dxCommon     = DirectXCommon::GetInstance();
    auto* imguiManager = ImGuiManager::GetInstance();

    // === 场景管理 ===
    SceneManager sceneManager;                        // ★ 有全局淡入淡出与加载期黑屏
    auto* title = new TitleScene();
    title->SetSceneManager(&sceneManager);           // 把 SM 指针传给首场景
    sceneManager.SetNextScene(title, /*useTransition=*/false);

    while (true) {
        if (KamataEngine::Update()) { break; }       // 窗口消息/结束处理

        // ESC 切换全屏
        auto* input = Input::GetInstance();
        if (input->TriggerKey(DIK_ESCAPE)) {
            isFullscreen = !isFullscreen;
            ApplyFullscreen(hwnd, isFullscreen, windowedStyle, windowedRect);
        }

        // === 帧逻辑 ===
        imguiManager->Begin();                       // ImGui begin
        sceneManager.Update();                       // ★ 更新当前场景（含过渡状态机）

        dxCommon->PreDraw();                         // 框架 2D/3D 绘制开始
        sceneManager.Draw();                         // ★ 先画场景，再由 SM 画最上层黑幕
        imguiManager->End();
        imguiManager->Draw();                        // ImGui 绘制
        dxCommon->PostDraw();                        // 呈现
    }
    KamataEngine::Finalize();
    return 0;
}
