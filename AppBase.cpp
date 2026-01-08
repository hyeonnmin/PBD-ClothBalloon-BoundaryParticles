#include "AppBase.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <dxgi.h>    // DXGIFactory
#include <dxgi1_4.h> // DXGIFactory4


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

namespace jhm {

    using namespace std;

    AppBase* g_appBase = nullptr;


    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

        return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
    }

    AppBase::AppBase()
        : m_screenWidth(1280), m_screenHeight(960), m_mainWindow(0),
        m_screenViewport(D3D11_VIEWPORT()) {

        g_appBase = this;
    }

    AppBase::~AppBase() {
        g_appBase = nullptr;

        // Cleanup
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        DestroyWindow(m_mainWindow);
    }

    float AppBase::GetAspectRatio() const {
        return float(m_screenWidth) / m_screenHeight;
    }

    int AppBase::Run() {

        // Main message loop
        MSG msg = { 0 };
        while (WM_QUIT != msg.message) {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();

                ImGui::NewFrame(); 
                ImGui::Begin("Scene Control");

                ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

                UpdateGUI(); 

                ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));

                m_guiWidth = int(ImGui::GetWindowWidth());

                ImGui::End();
                ImGui::Render();

                Update(ImGui::GetIO().DeltaTime);

                Render(); 

                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

                m_swapChain->Present(1, 0);
            }
        }

        return 0;
    }

    bool AppBase::Initialize() {

        if (!InitMainWindow())
            return false;

        if (!InitDirect3D())
            return false;

        if (!InitGUI())
            return false;

        return true;
    }

    LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;

        switch (msg) {
        case WM_SIZE:
            // Reset and resize swapchain
            // std::cout << (UINT)LOWORD(lParam) << " " << (UINT)HIWORD(lParam)
            //          << std::endl;

            if (m_swapChain) {

                m_screenWidth = int(LOWORD(lParam));
                m_screenHeight = int(HIWORD(lParam));
                m_guiWidth = 0;

                m_renderTargetView.Reset();
                m_swapChain->ResizeBuffers(0, 
                    (UINT)LOWORD(lParam),
                    (UINT)HIWORD(lParam),
                    DXGI_FORMAT_UNKNOWN, 
                    0);
                CreateRenderTargetView();
                CreateDepthBuffer();
                SetViewport();
            }

            break;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_MOUSEMOVE:
            // cout << "Mouse " << LOWORD(lParam) << " " << HIWORD(lParam) << endl;
            break;
        case WM_LBUTTONUP:
            // cout << "WM_LBUTTONUP Left mouse button" << endl;
            break;
        case WM_RBUTTONUP:
            // cout << "WM_RBUTTONUP Right mouse button" << endl;
            break;
        case WM_KEYDOWN:
             cout << "WM_KEYDOWN " << (int)wParam << endl;
             if ((int)wParam == 74)
             {
                 cout << "j_wind" << endl;
                 Wind(0);
             }
             else if ((int)wParam == 76)
             {
                 cout << "l_wind" << endl;
                 Wind(1);
             }
             else if ((int)wParam == 38)
             {
                 //cout << "w_volume" << endl;
                 ChangeVolume(0);
             }
             else if ((int)wParam == 40)
             {
                 //cout << "s_volume" << endl;
                 ChangeVolume(1);
             }
             else if ((int)wParam == 87)
                 ChangeView(87);
             else if ((int)wParam == 65)
                 ChangeView(65);
             else if ((int)wParam == 83)
                 ChangeView(83);
             else if ((int)wParam == 68)
                 ChangeView(68);
             else if ((int)wParam == 107)
                 ChangeView(107);
             else if ((int)wParam == 109)
                 ChangeView(109);
             else if ((int)wParam == 69)
                 ChangeView(69);
             else if ((int)wParam == 81)
                 ChangeView(81);

            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        }

        return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }

    bool AppBase::InitMainWindow() {

        WNDCLASSEX wc = { sizeof(WNDCLASSEX),
                         CS_CLASSDC,
                         WndProc,
                         0L,
                         0L,
                         GetModuleHandle(NULL),
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         L"Cloth_Balloons", 
                         NULL };

        // The RegisterClass function has been superseded by the RegisterClassEx
        // function.
        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerclassa?redirectedfrom=MSDN
        if (!RegisterClassEx(&wc)) {
            cout << "RegisterClassEx() failed." << endl;
            return false;
        }


        RECT wr = { 0, 0, m_screenWidth, m_screenHeight };


        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);

        m_mainWindow = CreateWindow(wc.lpszClassName, L"Cloth_Balloons",
            WS_OVERLAPPEDWINDOW,
            100, 
            100, 
            wr.right - wr.left, 
            wr.bottom - wr.top, 
            NULL, NULL, wc.hInstance, NULL);

        if (!m_mainWindow) {
            cout << "CreateWindow() failed." << endl;
            return false;
        }

        ShowWindow(m_mainWindow, SW_SHOWDEFAULT);
        UpdateWindow(m_mainWindow);

        return true;
    }

    bool AppBase::InitDirect3D() {

        const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;


        UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;

        const D3D_FEATURE_LEVEL featureLevels[2] = {
            D3D_FEATURE_LEVEL_11_0, 
            D3D_FEATURE_LEVEL_9_3 };
        D3D_FEATURE_LEVEL featureLevel;

        if (FAILED(D3D11CreateDevice(
            nullptr,    // Specify nullptr to use the default adapter.
            driverType, // Create a device using the hardware graphics driver.
            0, // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
            createDeviceFlags, // Set debug and Direct2D compatibility flags.
            featureLevels,     // List of feature levels this app can support.
            ARRAYSIZE(featureLevels), // Size of the list above.
            D3D11_SDK_VERSION,     // Always set this to D3D11_SDK_VERSION for
            // Microsoft Store apps.
            device.GetAddressOf(), // Returns the Direct3D device created.
            &featureLevel,         // Returns feature level of device created.
            context.GetAddressOf() // Returns the device immediate context.
        ))) {
            cout << "D3D11CreateDevice() failed." << endl;
            return false;
        }


        if (featureLevel != D3D_FEATURE_LEVEL_11_0) {
            cout << "D3D Feature Level 11 unsupported." << endl;
            return false;
        }

      
        device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4,
            &numQualityLevels);
        if (numQualityLevels <= 0) {
            cout << "MSAA not supported." << endl;
        }

        if (FAILED(device.As(&m_device))) {
            cout << "device.AS() failed." << endl;
            return false;
        }

        if (FAILED(context.As(&m_context))) {
            cout << "context.As() failed." << endl;
            return false;
        }

        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferDesc.Width = m_screenWidth;   // set the back buffer width
        sd.BufferDesc.Height = m_screenHeight; // set the back buffer height
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // use 32-bit color
        sd.BufferCount = 2;                                // Double-buffering
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage =
            DXGI_USAGE_RENDER_TARGET_OUTPUT; // how swap chain is to be used
        sd.OutputWindow = m_mainWindow;      // the window to be used
        sd.Windowed = TRUE;                  // windowed/full-screen mode
        sd.Flags =
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        if (numQualityLevels > 0) {
            sd.SampleDesc.Count = 4; // how many multisamples
            sd.SampleDesc.Quality = numQualityLevels - 1;
        }
        else {
            sd.SampleDesc.Count = 1; // how many multisamples
            sd.SampleDesc.Quality = 0;
        }

        if (FAILED(D3D11CreateDeviceAndSwapChain(
            0, // Default adapter
            driverType,
            0, // No software device
            createDeviceFlags, featureLevels, 1, D3D11_SDK_VERSION, &sd,
            m_swapChain.GetAddressOf(), m_device.GetAddressOf(), &featureLevel,
            m_context.GetAddressOf()))) {
            cout << "D3D11CreateDeviceAndSwapChain() failed." << endl;
            return false;
        }

        CreateRenderTargetView();

        SetViewport();

        // Create a rasterizer state
        D3D11_RASTERIZER_DESC rastDesc;
        ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC)); // Need this
        rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
        //rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
        rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
        rastDesc.FrontCounterClockwise = false;
        rastDesc.DepthClipEnable = true; // 

        m_device->CreateRasterizerState(&rastDesc,
            m_solidRasterizerSate.GetAddressOf());

        rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;

        m_device->CreateRasterizerState(&rastDesc,
            m_wireRasterizerSate.GetAddressOf());

        CreateDepthBuffer();

        // Create depth stencil state
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
        // depth off
        depthStencilDesc.DepthEnable = false; // false
        depthStencilDesc.DepthWriteMask =
            D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc =
            D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
        if (FAILED(m_device->CreateDepthStencilState(
            &depthStencilDesc, m_depthStencilState.GetAddressOf()))) {
            cout << "CreateDepthStencilState() failed." << endl;
        }

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(m_device->CreateBlendState(
            &blendDesc, m_blendState.GetAddressOf()))) {
            cout << "CreateBlendState() failed." << endl;
        }

        return true;
    }

    bool AppBase::InitGUI() {

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.DisplaySize = ImVec2(float(m_screenWidth), float(m_screenHeight));
        ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        if (!ImGui_ImplDX11_Init(m_device.Get(), m_context.Get())) {
            return false;
        }

        if (!ImGui_ImplWin32_Init(m_mainWindow)) {
            return false;
        }

        return true;
    }

    void AppBase::SetViewport() {

        static int previousGuiWidth = m_guiWidth;

        if (previousGuiWidth != m_guiWidth) {

            previousGuiWidth = m_guiWidth;

            // Set the viewport
            ZeroMemory(&m_screenViewport, sizeof(D3D11_VIEWPORT));
            m_screenViewport.TopLeftX = 0;
            m_screenViewport.TopLeftY = 0;
            m_screenViewport.Width = float(m_screenWidth);
            m_screenViewport.Height = float(m_screenHeight);
            m_screenViewport.MinDepth = 0.0f;
            m_screenViewport.MaxDepth = 1.0f; // Note: important for depth buffering

            m_context->RSSetViewports(1, &m_screenViewport);
        }
    }

    bool AppBase::CreateRenderTargetView() {

        ComPtr<ID3D11Texture2D> backBuffer;
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        if (backBuffer) {
            m_device->CreateRenderTargetView(backBuffer.Get(), NULL,
                m_renderTargetView.GetAddressOf());
        }
        else {
            std::cout << "CreateRenderTargetView() failed." << std::endl;
            return false;
        }

        return true;
    }

    bool AppBase::CreateDepthBuffer() {
        D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
        depthStencilBufferDesc.Width = m_screenWidth;
        depthStencilBufferDesc.Height = m_screenHeight;
        depthStencilBufferDesc.MipLevels = 1;
        depthStencilBufferDesc.ArraySize = 1;
        depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        if (numQualityLevels > 0) {
            depthStencilBufferDesc.SampleDesc.Count = 4; // how many multisamples
            depthStencilBufferDesc.SampleDesc.Quality = numQualityLevels - 1;
        }
        else {
            depthStencilBufferDesc.SampleDesc.Count = 1; // how many multisamples
            depthStencilBufferDesc.SampleDesc.Quality = 0;
        }
        depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilBufferDesc.CPUAccessFlags = 0;
        depthStencilBufferDesc.MiscFlags = 0;

        if (FAILED(m_device->CreateTexture2D(
            &depthStencilBufferDesc, 0, m_depthStencilBuffer.GetAddressOf()))) {
            std::cout << "CreateTexture2D() failed." << std::endl;
        }
        if (FAILED(m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), 0,
            &m_depthStencilView))) {
            std::cout << "CreateDepthStencilView() failed." << std::endl;
        }
        return true;
    }


    void CheckResult(HRESULT hr, ID3DBlob* errorBlob) {
        if (FAILED(hr)) {
            if ((hr & D3D11_ERROR_FILE_NOT_FOUND) != 0) {
                cout << "File not found." << endl;
            }

            if (errorBlob) {
                cout << "Shader compile error\n"
                    << (char*)errorBlob->GetBufferPointer() << endl;
            }
        }
    }


    void AppBase::CreateVertexShaderAndInputLayout(
        const wstring& filename,
        const vector<D3D11_INPUT_ELEMENT_DESC>& inputElements,
        ComPtr<ID3D11VertexShader>& vertexShader,
        ComPtr<ID3D11InputLayout>& inputLayout) {

        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr = D3DCompileFromFile(
            filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
            "vs_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

        CheckResult(hr, errorBlob.Get());

        m_device->CreateVertexShader(shaderBlob->GetBufferPointer(),
            shaderBlob->GetBufferSize(), NULL,
            &vertexShader);

        m_device->CreateInputLayout(inputElements.data(),
            UINT(inputElements.size()),
            shaderBlob->GetBufferPointer(),
            shaderBlob->GetBufferSize(), &inputLayout);
    }

    void AppBase::CreateGeometryShader(const wstring& filename,
        ComPtr<ID3D11GeometryShader>& geometryShader) {
        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif


        HRESULT hr = D3DCompileFromFile(
            filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
            "gs_5_0", compileFlags, 0, &shaderBlob, &errorBlob);


        CheckResult(hr, errorBlob.Get());

        m_device->CreateGeometryShader(shaderBlob->GetBufferPointer(),
            shaderBlob->GetBufferSize(), NULL,
            &geometryShader);
    }

    void AppBase::CreatePixelShader(const wstring& filename,
        ComPtr<ID3D11PixelShader>& pixelShader) {
        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

      
        HRESULT hr = D3DCompileFromFile(
            filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
            "ps_5_0", compileFlags, 0, &shaderBlob, &errorBlob);

        CheckResult(hr, errorBlob.Get());

        m_device->CreatePixelShader(shaderBlob->GetBufferPointer(),
            shaderBlob->GetBufferSize(), NULL,
            &pixelShader);
    }

    void AppBase::CreateIndexBuffer(const std::vector<uint32_t>& indices,
        ComPtr<ID3D11Buffer>& indexBuffer) {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; 
        bufferDesc.ByteWidth = UINT(sizeof(uint32_t) * indices.size());
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0; // 0 if no CPU access is necessary.
        bufferDesc.StructureByteStride = sizeof(uint32_t);

        D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
        indexBufferData.pSysMem = indices.data();
        indexBufferData.SysMemPitch = 0;
        indexBufferData.SysMemSlicePitch = 0;

        m_device->CreateBuffer(&bufferDesc, &indexBufferData,
            indexBuffer.GetAddressOf());
    }

    void AppBase::CreateTexture(
        const std::string filename, ComPtr<ID3D11Texture2D>& texture,
        ComPtr<ID3D11ShaderResourceView>& textureResourceView) {

        int width, height, channels;

        unsigned char* img =
            stbi_load(filename.c_str(), &width, &height, &channels, 0);

        std::vector<uint8_t> image;
        image.resize(width * height * 4);
        for (size_t i = 0; i < width * height; i++) {
            for (size_t c = 0; c < 3; c++) {
                image[4 * i + c] = img[i * channels + c];
            }
            image[4 * i + 3] = 255;
        }

        // Create texture.
        D3D11_TEXTURE2D_DESC txtDesc = {};
        txtDesc.Width = width;
        txtDesc.Height = height;
        txtDesc.MipLevels = txtDesc.ArraySize = 1;
        txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        txtDesc.SampleDesc.Count = 1;
        txtDesc.Usage = D3D11_USAGE_IMMUTABLE;
        txtDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        // Fill in the subresource data.
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = image.data();
        InitData.SysMemPitch = txtDesc.Width * sizeof(uint8_t) * 4;
        // InitData.SysMemSlicePitch = 0;

        m_device->CreateTexture2D(&txtDesc, &InitData, texture.GetAddressOf());
        m_device->CreateShaderResourceView(texture.Get(), nullptr,
            textureResourceView.GetAddressOf());
    }

}