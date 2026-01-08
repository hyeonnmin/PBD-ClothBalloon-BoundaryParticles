#pragma once

#include <wrl.h>

#include "GeometryGenerator.h"
#include "Material.h"
#include "Vertex.h"

namespace jhm {

    using Microsoft::WRL::ComPtr;

    struct CubeMapping {

        std::shared_ptr<Mesh> cubeMesh;

        ComPtr<ID3D11ShaderResourceView> cubemapResourceView;

        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11InputLayout> inputLayout;
    };
} 
