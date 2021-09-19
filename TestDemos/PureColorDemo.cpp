/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#include "Application.h"
#include "ApplicationException.h"

#include <DirectXColors.h>

class TestApp : public Application {
public:
    TestApp(HINSTANCE hInstance) : Application(hInstance) {}
    ~TestApp() {}

protected:
    void Update() override;
    void Draw() override;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    try {
        TestApp app(hInstance);
        return app.Run();
    }
    catch (ApplicationException& appException) {
        MessageBoxA(nullptr, appException.what(), "HR Error", MB_OK | MB_ICONERROR);
    }
}

void TestApp::Update() {}

void TestApp::Draw() {
    ClearCurrentRenderTargetView(DirectX::Colors::LightSteelBlue);
    ClearCurrentDepthStencilView(1.0f, 0);
}