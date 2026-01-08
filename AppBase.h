#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>
#include <windows.h>
#include <wrl.h> // ComPtr

namespace jhm {

    using Microsoft::WRL::ComPtr;
    using std::shared_ptr;
    using std::vector;
    using std::wstring;


    class AppBase {
    public:
        AppBase();
        virtual ~AppBase();

        float GetAspectRatio() const;

        int Run();

        virtual bool Initialize();
        virtual void UpdateGUI() = 0;
        virtual void Update(float dt) = 0;
        virtual void Render() = 0;
        virtual void Wind(int key) = 0;
        virtual void ChangeVolume(int key) = 0;
        virtual void ChangeView(int key) = 0;

        virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // Convenience overrides for handling mouse input.
        virtual void OnMouseDown(WPARAM btnState, int x, int y) {};
        virtual void OnMouseUp(WPARAM btnState, int x, int y) {};
        virtual void OnMouseMove(WPARAM btnState, int x, int y) {};

    protected: 
        bool InitMainWindow();
        bool InitDirect3D();
        bool InitGUI();

        void SetViewport();
        bool CreateRenderTargetView();
        bool CreateDepthBuffer();

        void CreateVertexShaderAndInputLayout(
            const wstring& filename,
            const vector<D3D11_INPUT_ELEMENT_DESC>& inputElements,
            ComPtr<ID3D11VertexShader>& vertexShader,
            ComPtr<ID3D11InputLayout>& inputLayout);
        void CreateGeometryShader(const wstring& filename,
            ComPtr<ID3D11GeometryShader>& geometryShader);
        void CreatePixelShader(const wstring& filename,
            ComPtr<ID3D11PixelShader>& pixelShader);
        void CreateIndexBuffer(const vector<uint32_t>& indices,
            ComPtr<ID3D11Buffer>& indexBuffer);

        // PBD simulation시 사용되는 create vertex buffer (vertex 변화 O)
        template <typename T_VERTEX>
        void CreateVertexBuffer(const vector<T_VERTEX>& vertices,
            ComPtr<ID3D11Buffer>& vertexBuffer) {

            // D3D11_USAGE enumeration (d3d11.h)
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage

            D3D11_BUFFER_DESC bufferDesc;
            ZeroMemory(&bufferDesc, sizeof(bufferDesc));
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            // Particle 개수 변화에 맞게 buffer size 넉넉히 잡음
            bufferDesc.ByteWidth = UINT(sizeof(T_VERTEX) * (vertices.size() * 1000));
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // 0 if no CPU access is necessary.
            bufferDesc.MiscFlags = 0;
            bufferDesc.StructureByteStride = sizeof(T_VERTEX);

            D3D11_SUBRESOURCE_DATA vertexBufferData = {
                0 };
            vertexBufferData.pSysMem = vertices.data();
            vertexBufferData.SysMemPitch = 0;
            vertexBufferData.SysMemSlicePitch = 0;

            // 초기 버퍼 입력도 NULL로 (vertexBuffer를 넣으면 buffer size 넉넉히 잡은게 소용x)
            const HRESULT hr = m_device->CreateBuffer(
                &bufferDesc, NULL, vertexBuffer.GetAddressOf());
            if (FAILED(hr)) {
                std::cout << "CreateBuffer() failed. " << std::hex << hr
                    << std::endl;
            };
        }


        // 일반적인 create vertex buffer (vertex 변화 X)
        template <typename T_VERTEX>
        void SubCreateVertexBuffer(const vector<T_VERTEX>& vertices,
            ComPtr<ID3D11Buffer>& vertexBuffer) {

            // D3D11_USAGE enumeration (d3d11.h)
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage

            D3D11_BUFFER_DESC bufferDesc;
            ZeroMemory(&bufferDesc, sizeof(bufferDesc));
            bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 초기화 후 변경X
            bufferDesc.ByteWidth = UINT(sizeof(T_VERTEX) * vertices.size());
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bufferDesc.CPUAccessFlags = 0; // 0 if no CPU access is necessary.
            bufferDesc.StructureByteStride = sizeof(T_VERTEX);

            D3D11_SUBRESOURCE_DATA vertexBufferData = {
                0 }; // MS 예제에서 초기화하는 방식
            vertexBufferData.pSysMem = vertices.data();
            vertexBufferData.SysMemPitch = 0;
            vertexBufferData.SysMemSlicePitch = 0;

            const HRESULT hr = m_device->CreateBuffer(
                &bufferDesc, &vertexBufferData, vertexBuffer.GetAddressOf());
            if (FAILED(hr)) {
                std::cout << "CreateBuffer() failed. " << std::hex << hr
                    << std::endl;
            };
        }


        template <typename T_CONSTANT>
        void CreateConstantBuffer(const T_CONSTANT& constantBufferData,
            ComPtr<ID3D11Buffer>& constantBuffer) {

            D3D11_BUFFER_DESC cbDesc;
            cbDesc.ByteWidth = sizeof(constantBufferData);
            cbDesc.Usage = D3D11_USAGE_DYNAMIC;
            cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            cbDesc.MiscFlags = 0;
            cbDesc.StructureByteStride = 0;

            // Fill in the subresource data.
            D3D11_SUBRESOURCE_DATA initData;
            initData.pSysMem = &constantBufferData;
            initData.SysMemPitch = 0;
            initData.SysMemSlicePitch = 0;

            auto hr = m_device->CreateBuffer(&cbDesc, &initData,
                constantBuffer.GetAddressOf());
            if (FAILED(hr)) {
                std::cout << "CreateConstantBuffer() CreateBuffer failed()."
                    << std::endl;
            }
        }

        template <typename T_DATA>
        void UpdateBuffer(const T_DATA& bufferData, ComPtr<ID3D11Buffer>& buffer) {

            if (!buffer) {
                std::cout << "UpdateBuffer() buffer was not initialized."
                    << std::endl;
            }

            D3D11_MAPPED_SUBRESOURCE ms;
            m_context->Map(buffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
            memcpy(ms.pData, &bufferData, sizeof(bufferData));
            m_context->Unmap(buffer.Get(), NULL);
        }
        

        //PBD simulation시 사용되는 vertex update buffer
        template <typename T_DATA>
        void UpdateVertexBuffer(const T_DATA& bufferData, ComPtr<ID3D11Buffer>& buffer) {

            if (!buffer) {
                std::cout << "UpdateVertexBuffer() buffer was not initialized."
                    << std::endl;
            }

            D3D11_MAPPED_SUBRESOURCE ms;
            m_context->Map(buffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
            memcpy(ms.pData, bufferData.data(), bufferData.size() * sizeof(bufferData[0]));
            m_context->Unmap(buffer.Get(), NULL);
        }

        void CreateTexture(const std::string filename,
            ComPtr<ID3D11Texture2D>& texture,
            ComPtr<ID3D11ShaderResourceView>& textureResourceView);

    public:
        int m_screenWidth;
        int m_screenHeight;
        int m_guiWidth = 0;
        HWND m_mainWindow;
        UINT numQualityLevels = 0;

        ComPtr<ID3D11Device> m_device;
        ComPtr<ID3D11DeviceContext> m_context;
        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<IDXGISwapChain> m_swapChain;

        ComPtr<ID3D11RasterizerState> m_solidRasterizerSate;
        ComPtr<ID3D11RasterizerState> m_wireRasterizerSate;
        bool m_drawAsWire = false;

        // Depth buffer 관련
        ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;
        ComPtr<ID3D11BlendState> m_blendState;

        D3D11_VIEWPORT m_screenViewport;
    };
} 