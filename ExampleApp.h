#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include "AppBase.h"
#include "GeometryGenerator.h"
#include "Material.h"
#include "CubeMapping.h"

namespace jhm {

    using DirectX::SimpleMath::Matrix;
    using DirectX::SimpleMath::Vector2;
    using DirectX::SimpleMath::Vector3;
    using DirectX::SimpleMath::Vector4;

    struct Light {
        Vector3 strength = Vector3(1.0f);              // 12
        float fallOffStart = 0.0f;                     // 4
        Vector3 direction = Vector3(0.0f, 0.0f, 1.0f); // 12
        float fallOffEnd = 100.0f;                      // 4
        Vector3 position = Vector3(0.0f, 0.0f, -2.0f); // 12
        float spotPower = 1.0f;                        // 4
    };

    struct BasicVertexConstantBuffer {
        Matrix model;
        Matrix invTranspose;
        Matrix view;
        Matrix projection;
    };

    static_assert((sizeof(BasicVertexConstantBuffer) % 16) == 0,
        "Constant Buffer size must be 16-byte aligned");

    struct BasicGeometryConstantBuffer {
        Matrix model;
        Matrix invTranspose;
        Matrix view;
        Matrix projection;
        Vector3 gCamRightWS;
        float dummy1;
        Vector3 gCamUpWS;
        float scaling;
    };

    static_assert((sizeof(BasicGeometryConstantBuffer) % 16) == 0,
        "Constant Buffer size must be 16-byte aligned");

#define MAX_LIGHTS 3

    struct BasicPixelConstantBuffer {
        Vector3 eyeWorld;         // 12
        bool useTexture;          // 4
        Material material;        // 48
        Light lights[MAX_LIGHTS]; // 48 * MAX_LIGHTS
    };

    static_assert((sizeof(BasicPixelConstantBuffer) % 16) == 0,
        "Constant Buffer size must be 16-byte aligned");

    struct NormalVertexConstantBuffer {
        // Matrix model;
        // Matrix invTranspose;
        // Matrix view;
        // Matrix projection;
        float scale = 0.1f;
        float dummy[3];
    };
    
    class ExampleApp : public AppBase {
    public:
        ExampleApp();

        virtual bool Initialize() override;
        virtual void UpdateGUI() override;
        virtual void Update(float dt) override;
        virtual void Render() override;
        virtual void Wind(int key) override;
        virtual void ChangeVolume(int key) override;
        virtual void ChangeView(int key) override;

        void applyExtForces(float dt);
        void solveDistanceConstraint(int index0, int index1, float restlength);
        void solveDihedralConstraint(int index0, int index1, int index2, int index3, float restAngle);
        void computeConstraintScaling();
        void initVolume();
        void initParticle();
        void EdgeSampling(Edge& e, float d);
        void InnerSampling(Triangle& t, float d);
        void solveOverpressureConstraint(int index0, int index1, int index2);
        void updateNormal();
        void updateParticle();
        void integrate(float dt);
        void InitializeCubeMapping();
        void makePointCloudFile();
    protected:
        ComPtr<ID3D11VertexShader> m_basicVertexShader;
        ComPtr<ID3D11GeometryShader> m_basicGeometryShader;
        ComPtr<ID3D11PixelShader> m_basicPixelShader;
        ComPtr<ID3D11InputLayout> m_basicInputLayout;

        std::vector<shared_ptr<Mesh>> m_meshes;

        // Texturing
        ComPtr<ID3D11Texture2D> m_texture;
        ComPtr<ID3D11ShaderResourceView> m_textureResourceView;
        ComPtr<ID3D11SamplerState> m_samplerState;

        BasicVertexConstantBuffer m_BasicVertexConstantBufferData;
        BasicGeometryConstantBuffer m_BasicGeometryConstantBufferData;
        BasicPixelConstantBuffer m_BasicPixelConstantBufferData;

        // Volume constraint
        float m_pressure = 1.0f;
        float m_volume;
        float m_cur_volume;
        float m_scaling;

        bool m_usePerspectiveProjection = true;
        Vector3 m_modelTranslation = Vector3(0.0f);
        Vector3 m_modelRotation = Vector3(0.0f, 0.0f, 0.0f);
        Vector3 m_modelScaling = Vector3(0.8f);
        Vector3 m_viewTranslation = Vector3(0.0f, 0.0f, 8.0f);
        Vector3 m_viewRot = Vector3(0.0f);

        float m_projFovAngleY = 70.0f;
        float m_nearZ = 0.01f;
        float m_farZ = 100.0f;

        int m_lightType = 0;
        Light m_lightFromGUI;
        float m_materialDiffuse = 0.15f;
        float m_materialSpecular = 1.0f;
        float m_gaussian_scaling = 1.0f;

        vector<MeshData> m_meshData;

        // 노멀 벡터 그리기
        ComPtr<ID3D11VertexShader> m_normalVertexShader;
        ComPtr<ID3D11PixelShader> m_normalPixelShader;
        // ComPtr<ID3D11InputLayout> m_normalInputLayout; // 다른 쉐이더와 같이 사용

        shared_ptr<Mesh> m_normalLines;
        NormalVertexConstantBuffer m_normalVertexConstantBufferData;
        bool m_drawNormals = false;
        bool m_drawNormalsDirtyFlag = false;
        bool m_viewRotationY = false;

        CubeMapping m_cubeMapping;
    };
} 
