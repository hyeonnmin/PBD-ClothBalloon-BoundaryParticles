#include "ExampleApp.h"

#include <directxtk/DDSTextureLoader.h> // 큐브맵 읽을 때 필요
#include <directxtk/WICTextureLoader.h>
#include <tuple>
#include <vector>

// makePointCloudFile() 사용
#include <fstream>
#include <cmath>

#include "GeometryGenerator.h"
namespace jhm {

    using namespace std;
    using namespace DirectX;

    ExampleApp::ExampleApp() : AppBase(), m_BasicPixelConstantBufferData() {}

    void ExampleApp::InitializeCubeMapping() {

        // texassemble.exe cube -w 2048 -h 2048 -o saintpeters.dds posx.jpg negx.jpg
        // posy.jpg negy.jpg posz.jpg negz.jpg texassemble.exe cube -w 2048 -h 2048
        // -o skybox.dds right.jpg left.jpg top.jpg bottom.jpg front.jpg back.jpg -y
        // https://github.com/Microsoft/DirectXTex/wiki/Texassemble

        // .dds 파일 읽어들여서 초기화
        ComPtr<ID3D11Texture2D> texture;
        auto hr = CreateDDSTextureFromFileEx(
            this->m_device.Get(), L"C:/Users/wjdgu/source/repos/Particle_Cloth_Balloons/skybox/saintpeters.dds", 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0,
            D3D11_RESOURCE_MISC_TEXTURECUBE, // 큐브맵용 텍스춰
            DDS_LOADER_FLAGS(false), (ID3D11Resource**)texture.GetAddressOf(),
            this->m_cubeMapping.cubemapResourceView.GetAddressOf(), nullptr);

        if (FAILED(hr)) {
            std::cout << "CreateDDSTextureFromFileEx() failed" << std::endl;
        }

        m_cubeMapping.cubeMesh = std::make_shared<Mesh>();

        m_BasicVertexConstantBufferData.model = Matrix();
        m_BasicVertexConstantBufferData.view = Matrix();
        m_BasicVertexConstantBufferData.projection = Matrix();
        ComPtr<ID3D11Buffer> vertexConstantBuffer;
        ComPtr<ID3D11Buffer> pixelConstantBuffer;
        AppBase::CreateConstantBuffer(m_BasicVertexConstantBufferData,
            m_cubeMapping.cubeMesh->vertexConstantBuffer);
        AppBase::CreateConstantBuffer(m_BasicPixelConstantBufferData,
            m_cubeMapping.cubeMesh->pixelConstantBuffer);

        // 박스 초기화
        MeshData cubeMeshData = GeometryGenerator::MakeBox(20.0f);
        std::reverse(cubeMeshData.indices.begin(), cubeMeshData.indices.end());

        AppBase::SubCreateVertexBuffer(cubeMeshData.vertices,
            m_cubeMapping.cubeMesh->vertexBuffer);
        m_cubeMapping.cubeMesh->m_indexCount = UINT(cubeMeshData.indices.size());
        AppBase::CreateIndexBuffer(cubeMeshData.indices,
            m_cubeMapping.cubeMesh->indexBuffer);

        // 쉐이더 초기화

        // 다른 쉐이더와 동일한 InputLayout
        // 실제로는 "POSITION"만 사용
        vector<D3D11_INPUT_ELEMENT_DESC> basicInputElements = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        AppBase::CreateVertexShaderAndInputLayout(
            L"CubeMappingVertexShader.hlsl", basicInputElements,
            m_cubeMapping.vertexShader, m_cubeMapping.inputLayout);

        AppBase::CreatePixelShader(L"CubeMappingPixelShader.hlsl",
            m_cubeMapping.pixelShader);

        // 기타
        // - 텍스춰 샘플러도 다른 텍스춰와 같이 사용
    }

    bool ExampleApp::Initialize() {

        if (!AppBase::Initialize())
            return false;

        InitializeCubeMapping();

        AppBase::CreateTexture("ojwD8.jpg", m_texture, m_textureResourceView);

        // Texture sampler 만들기
        D3D11_SAMPLER_DESC sampDesc;
        ZeroMemory(&sampDesc, sizeof(sampDesc));
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

        // Create the Sample State
        m_device->CreateSamplerState(&sampDesc, m_samplerState.GetAddressOf());

        //m_meshData = GeometryGenerator::ReadFromFile(
        //    "C:/Users/wjdgu/Desktop/", "object1.obj");
        m_meshData.push_back(GeometryGenerator::MakeSphere(2.5, 30, 30));
        //m_meshData.push_back(GeometryGenerator::MakeBox(2.5f));

        initParticle();
        //makePointCloudFile();

        m_BasicVertexConstantBufferData.model = Matrix();
        m_BasicVertexConstantBufferData.view = Matrix();
        m_BasicVertexConstantBufferData.projection = Matrix();

        m_BasicGeometryConstantBufferData.model = Matrix();
        m_BasicGeometryConstantBufferData.view = Matrix();
        m_BasicGeometryConstantBufferData.projection = Matrix();
        m_BasicGeometryConstantBufferData.gCamRightWS = Vector3();
        m_BasicGeometryConstantBufferData.gCamUpWS = Vector3();

        ComPtr<ID3D11Buffer> vertexConstantBuffer;
        ComPtr<ID3D11Buffer> geometryConstantBuffer;
        ComPtr<ID3D11Buffer> pixelConstantBuffer;

        AppBase::CreateConstantBuffer(m_BasicVertexConstantBufferData,
            vertexConstantBuffer);
        AppBase::CreateConstantBuffer(m_BasicGeometryConstantBufferData,
            geometryConstantBuffer);
        AppBase::CreateConstantBuffer(m_BasicPixelConstantBufferData,
            pixelConstantBuffer);

        for (const auto& meshData : m_meshData) {
            auto newMesh = std::make_shared<Mesh>();

            // paricle vertices 와 vertices 합치기
            m_meshData[0].totalVertices.insert(m_meshData[0].totalVertices.end(), m_meshData[0].vertices.begin(), m_meshData[0].vertices.end());
            m_meshData[0].totalVertices.insert(m_meshData[0].totalVertices.end(), m_meshData[0].particleVertices.begin(), m_meshData[0].particleVertices.end());
            cout << m_meshData[0].totalVertices.size() << endl;
            AppBase::CreateVertexBuffer(meshData.totalVertices, newMesh->vertexBuffer);
            newMesh->m_indexCount = UINT(meshData.totalVertices.size());
            AppBase::CreateIndexBuffer(meshData.indices, newMesh->indexBuffer);

            if (!meshData.textureFilename.empty()) {

                cout << meshData.textureFilename << endl;
                AppBase::CreateTexture(meshData.textureFilename, newMesh->texture,
                    newMesh->textureResourceView);
            }

            newMesh->vertexConstantBuffer = vertexConstantBuffer;
            newMesh->geometryConstantBuffer = geometryConstantBuffer;
            newMesh->pixelConstantBuffer = pixelConstantBuffer;

            this->m_meshes.push_back(newMesh);
        }

        vector<D3D11_INPUT_ELEMENT_DESC> basicInputElements = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        AppBase::CreateVertexShaderAndInputLayout(
            L"BasicVertexShader.hlsl", basicInputElements, m_basicVertexShader,
            m_basicInputLayout);
        AppBase::CreateGeometryShader(L"BasicGeometryShader.hlsl", m_basicGeometryShader);
        AppBase::CreatePixelShader(L"BasicPixelShader.hlsl", m_basicPixelShader);

        // 노멀 벡터 그리기
        // 문제를 단순화하기 위해 InputLayout은 BasicVertexShader와 같이 사용합시다.
        m_normalLines = std::make_shared<Mesh>();

        std::vector<Vertex> normalVertices;
        std::vector<uint32_t> normalIndices;

        // 여러 메쉬의 normal 들을 하나로 합치기
        size_t offset = 0;
        for (const auto& meshData : m_meshData) {
            for (size_t i = 0; i < meshData.totalVertices.size(); i++) {

                auto v = meshData.totalVertices[i];

                v.texcoord.x = 0.0f; // 시작점 표시
                normalVertices.push_back(v);

                v.texcoord.x = 1.0f; // 끝점 표시
                normalVertices.push_back(v);

                normalIndices.push_back(uint32_t(2 * (i + offset)));
                normalIndices.push_back(uint32_t(2 * (i + offset) + 1));
            }
            offset += meshData.totalVertices.size();
        }

        AppBase::SubCreateVertexBuffer(normalVertices, m_normalLines->vertexBuffer);
        m_normalLines->m_indexCount = UINT(normalIndices.size());
        AppBase::CreateIndexBuffer(normalIndices, m_normalLines->indexBuffer);
        AppBase::CreateConstantBuffer(m_normalVertexConstantBufferData,
            m_normalLines->vertexConstantBuffer);

        AppBase::CreateVertexShaderAndInputLayout(
            L"NormalVertexShader.hlsl", basicInputElements, m_normalVertexShader,
            m_basicInputLayout);
        AppBase::CreatePixelShader(L"NormalPixelShader.hlsl", m_normalPixelShader);

        

        initVolume();

        return true;
    }

    void ExampleApp::applyExtForces(float dt)
    {
        Vector3 gravity(0.0f, -9.8f, 0.0f);
        float damping = 0.59f;
        //초기(평형) 상태(물체의 무게 * 중력  = 부력) 가정
        float density = 1/m_volume;
        float dragCoefficient = 0.47f;
        float dragArea = 10.0f;
        for (auto& meshData : m_meshData) {

            for (int i = 0; i < meshData.vertices.size(); ++i)
            {
                Vertex v1 = meshData.vertices[i];
                Vertex2 v2 = meshData.vertices2[i];
                Vector3 pos = v1.position;
                Vector3 vel = v2.velocity;
                Vector3 velNorm = v2.velocity;
                velNorm.Normalize();

                ////중력 계산
                //vel += gravity * dt;
                ////부력 계산
                //Vector3 buoyancy = -gravity * m_cur_volume * density * v2.invMass;
                //vel += buoyancy * dt;
                ////항력 계산
                //Vector3 drag = - 0.5 * density * vel.Length() * vel.Length() * dragArea * dragCoefficient * velNorm * v2.invMass;
                //vel += drag * dt;

                vel *= damping;
                v2.newPosition = pos + vel * dt;

                meshData.vertices2[i].velocity = vel;
                meshData.vertices2[i].newPosition = v2.newPosition;

            }
        }
    }

    void ExampleApp::solveDistanceConstraint(int index0, int index1, float restlength)
    {
        float k = 0.9f;

        Vector3 pos1 = m_meshData[0].vertices2[index0].newPosition;
        Vector3 pos2 = m_meshData[0].vertices2[index1].newPosition;
        float invMass1 = m_meshData[0].vertices2[index0].invMass;
        float invMass2 = m_meshData[0].vertices2[index1].invMass;

        float c_p1p2 = (pos1 - pos2).Length() - restlength;
 /*       if (index0 == 0)
            cout << "Distance Constraint : " << c_p1p2 << endl;*/

        Vector3 dp1 = (pos1 - pos2);
        Vector3 dp2 = (pos1 - pos2);
        dp1.Normalize();
        dp2.Normalize();
        dp1 *= -invMass1 / (invMass1 + invMass2) * c_p1p2;
        dp2 *= invMass2 / (invMass1 + invMass2) * c_p1p2;
        pos1 += k * dp1;
        pos2 += k * dp2;

        m_meshData[0].vertices2[index0].newPosition = pos1;
        m_meshData[0].vertices2[index1].newPosition = pos2;
        
        //누적 후, 한 번에 업데이트
        //m_meshData[0].vertices2[index0].gradPosition += k * dp1;
        //m_meshData[0].vertices2[index1].gradPosition += k * dp2;
    }

    void ExampleApp::solveDihedralConstraint(int index0, int index1, int index2, int index3, float restAngle)
    {
        float stiffness = 1.0f;
        Vector3 pos0 = m_meshData[0].vertices2[index0].newPosition;
        Vector3 pos1 = m_meshData[0].vertices2[index1].newPosition;
        Vector3 pos2 = m_meshData[0].vertices2[index2].newPosition;
        Vector3 pos3 = m_meshData[0].vertices2[index3].newPosition;

        Vector3 e = pos3 - pos2;
        float length = e.Length();
        if (length < 0.001) {
            return;
        }
        float invlength = 1.0 / length;
        Vector3 n1 = (pos3 - pos0).Cross(pos2 - pos0);
        Vector3 n2 = (pos2 - pos1).Cross(pos3 - pos1);
        n1 /= n1.LengthSquared();
        n2 /= n2.LengthSquared();

        if (n1.LengthSquared() < 0.001 || n2.LengthSquared()) {
            return;
        }

        Vector3 d0 = n1 * length;
        Vector3 d1 = n2 * length;
        Vector3 d2 = n1 * ((pos0 - pos3).Dot(e) * invlength) + n2 * ((pos1 - pos3).Dot(e) * invlength);
        Vector3 d3 = n1 * ((pos2 - pos0).Dot(e) * invlength) + n2 * ((pos2 - pos1).Dot(e) * invlength);

        n1.Normalize();
        n2.Normalize();
        float dot = n1.Dot(n2);

        if (dot < -1.0) {
            dot = -1.0;
        }
        if (dot > 1.0) {
            dot = 1.0;
        }
        float phi = acos(dot);

        float lambda = m_meshData[0].vertices2[index0].invMass * d0.LengthSquared() +
            m_meshData[0].vertices2[index1].invMass * d1.LengthSquared() +
            m_meshData[0].vertices2[index2].invMass * d2.LengthSquared() +
            m_meshData[0].vertices2[index3].invMass * d3.LengthSquared();

        if (lambda == 0.0) {
            return;
        }

        lambda = (phi - restAngle) / lambda * stiffness;

        if (n2.Cross(n1).Dot(e) > 0.0) {
            lambda = -lambda;
        }

        m_meshData[0].vertices2[index0].newPosition += d0 * (-m_meshData[0].vertices2[index0].invMass * lambda);
        m_meshData[0].vertices2[index1].newPosition += d1 * (-m_meshData[0].vertices2[index1].invMass * lambda);
        m_meshData[0].vertices2[index2].newPosition += d2 * (-m_meshData[0].vertices2[index2].invMass * lambda);
        m_meshData[0].vertices2[index3].newPosition += d3 * (-m_meshData[0].vertices2[index3].invMass * lambda);

        //누적 후, 한 번에 업데이트
        //m_meshData[0].vertices2[index0].gradPosition += d0 * (-m_meshData[0].vertices2[index0].invMass * lambda);
        //m_meshData[0].vertices2[index1].gradPosition += d1 * (-m_meshData[0].vertices2[index1].invMass * lambda);
        //m_meshData[0].vertices2[index2].gradPosition += d2 * (-m_meshData[0].vertices2[index2].invMass * lambda);
        //m_meshData[0].vertices2[index3].gradPosition += d3 * (-m_meshData[0].vertices2[index3].invMass * lambda);

        double visLambda = fabs(lambda * 1000000.0);
        m_meshData[0].vertices2[index0].bendingForce += visLambda;
        m_meshData[0].vertices2[index1].bendingForce += visLambda;
        m_meshData[0].vertices2[index2].bendingForce += visLambda;
        m_meshData[0].vertices2[index3].bendingForce += visLambda;


    }

    void ExampleApp::initVolume()
    {
        float volume = 0.0f;

        for (int i = 0; i < m_meshData[0].indices.size(); i = i + 3)
        {
            int index0 = m_meshData[0].indices[i];
            int index1 = m_meshData[0].indices[i + 1];
            int index2 = m_meshData[0].indices[i + 2];

            Vector3 pos0 = m_meshData[0].vertices[index0].position;
            Vector3 pos1 = m_meshData[0].vertices[index1].position;
            Vector3 pos2 = m_meshData[0].vertices[index2].position;

            volume += pos0.Cross(pos1).Dot(pos2);

        }


        m_volume = volume;
        m_cur_volume = volume;
    }

    void ExampleApp::EdgeSampling(Edge &e, float d)
    {
        if (e.visited == false)
        {
            int index0 = e.index0;
            int index1 = e.index1;

            Vector3 pos0 = m_meshData[0].vertices[index0].position;
            Vector3 pos1 = m_meshData[0].vertices[index1].position;

            float l = (pos0 - pos1).Length();

            int n = (int)std::floor(l / d);

            Vector3 p = (pos0 - pos1) / n;

            Vertex v;
            // update
            if (e.n == n)
            {
                for (int i = 1; i < n; ++i)
                {
                    e.edgeParticles[i - 1].position = pos1 + p * i;
                    e.edgeParticles[i - 1].normal 
                        = (m_meshData[0].vertices[index1].normal * (n-i) + m_meshData[0].vertices[index0].normal * i);
                    e.edgeParticles[i - 1].normal.Normalize();
                    e.edgeParticles[i - 1].texcoord 
                        = (m_meshData[0].vertices[index1].texcoord * (n - i) + m_meshData[0].vertices[index0].texcoord * i);
                }
            }
            // resampling
            else
            {
                e.edgeParticles.clear();
                for (int i = 1; i < n; ++i)
                {
                    v.position = pos1 + p * i;
                    v.normal = (m_meshData[0].vertices[index1].normal * (n - i) + m_meshData[0].vertices[index0].normal * i);
                    v.normal.Normalize();
                    v.texcoord = (m_meshData[0].vertices[index1].texcoord * (n - i) + m_meshData[0].vertices[index0].texcoord * i);
                    e.edgeParticles.push_back(v);
                }
                e.n = n;
            }
            m_meshData[0].particleVertices.insert(m_meshData[0].particleVertices.end(), e.edgeParticles.begin(), e.edgeParticles.end());
            e.visited = true;
        }
    }

    void ExampleApp::InnerSampling(Triangle& t, float d)
    {
        // inner particle
        int index0 = t.vertexIndices[0];
        int index1 = t.vertexIndices[1];
        int index2 = t.vertexIndices[2];

        Vector3 pos0 = m_meshData[0].vertices[index0].position;
        Vector3 pos1 = m_meshData[0].vertices[index1].position;
        Vector3 pos2 = m_meshData[0].vertices[index2].position;

        float l0 = (pos0 - pos1).Length();
        float l1 = (pos1 - pos2).Length();
        float l2 = (pos2 - pos0).Length();

        Vector3 eLong;
        Vector3 eMid;
        Vector3 eShort;
        Vector3 mStart;
        Vector3 lStart;
        Vector3 p0;
        Vector3 p1;
        Vector3 p2;
        Vector3 n0;
        Vector3 n1;
        Vector3 n2;
        float lLong = max(max(l0, l1), l2);
        float lShort = min(min(l0, l1), l2);
        int shortEdgeIndex;


        if (lLong == l0)
        {
            if (lShort == l1)
            {
                eShort = pos2 - pos1;
                shortEdgeIndex = t.edgeIndices[1];
                eMid = pos0 - pos2;
                eLong = pos0 - pos1;
                mStart = pos2;
                p1 = m_meshData[0].vertices[index2].position;
                n1 = m_meshData[0].vertices[index2].normal;

                lStart = pos1;
                p2 = m_meshData[0].vertices[index1].position;
                p0 = m_meshData[0].vertices[index0].position;
                n2 = m_meshData[0].vertices[index1].normal;
                n0 = m_meshData[0].vertices[index0].normal;

            }
            else
            {
                eShort = pos2 - pos0;
                shortEdgeIndex = t.edgeIndices[2];
                eMid = pos1 - pos2;
                eLong = pos1 - pos0;
                mStart = pos2;
                p1 = m_meshData[0].vertices[index2].position;
                n1 = m_meshData[0].vertices[index2].normal;

                lStart = pos0;
                p2 = m_meshData[0].vertices[index0].position;
                p0 = m_meshData[0].vertices[index1].position;
                n2 = m_meshData[0].vertices[index0].normal;
                n0 = m_meshData[0].vertices[index1].normal;
            }
        }
        else if (lLong == l1)
        {
            if (lShort == l0)
            {
                eShort = pos0 - pos1;
                shortEdgeIndex = t.edgeIndices[0];
                eMid = pos2 - pos0;
                eLong = pos2 - pos1;
                mStart = pos0;
                p1 = m_meshData[0].vertices[index0].position;
                n1 = m_meshData[0].vertices[index0].normal;
                lStart = pos1;
                p2 = m_meshData[0].vertices[index1].position;
                p0 = m_meshData[0].vertices[index2].position;
                n2 = m_meshData[0].vertices[index1].normal;
                n0 = m_meshData[0].vertices[index2].normal;
            }
            else
            {
                eShort = pos0 - pos2;
                shortEdgeIndex = t.edgeIndices[2];
                eMid = pos1 - pos0;
                eLong = pos1 - pos2;
                mStart = pos0;
                p1 = m_meshData[0].vertices[index0].position;
                n1 = m_meshData[0].vertices[index0].normal;
                lStart = pos2;
                p2 = m_meshData[0].vertices[index2].position;
                p0 = m_meshData[0].vertices[index1].position;
                n2 = m_meshData[0].vertices[index2].normal;
                n0 = m_meshData[0].vertices[index1].normal;
            }
        }
        else
        {
            if (lShort == l0)
            {
                eShort = pos1 - pos0;
                shortEdgeIndex = t.edgeIndices[0];
                eMid = pos2 - pos1;
                eLong = pos2 - pos0;
                mStart = pos1;
                p1 = m_meshData[0].vertices[index1].position;
                n1 = m_meshData[0].vertices[index1].normal;

                lStart = pos0;
                p2 = m_meshData[0].vertices[index0].position;
                p0 = m_meshData[0].vertices[index2].position;
                n2 = m_meshData[0].vertices[index0].normal;
                n0 = m_meshData[0].vertices[index2].normal;
            }
            else
            {
                eShort = pos1 - pos2;
                shortEdgeIndex = t.edgeIndices[1];
                eMid = pos0 - pos1;
                eLong = pos0 - pos2;
                mStart = pos1;
                p1 = m_meshData[0].vertices[index1].position;
                n1 = m_meshData[0].vertices[index1].normal;
                lStart = pos2;
                p2 = m_meshData[0].vertices[index2].position;
                p0 = m_meshData[0].vertices[index0].position;
                n2 = m_meshData[0].vertices[index2].normal;
                n0 = m_meshData[0].vertices[index0].normal;
            }
        }

        Vector3 s = eShort.Cross(eLong.Cross(eShort));
        s.Normalize();
        float h = s.Dot(eLong);
        int nt = (int)std::floor(h / d);
        int ns = (int)std::floor(lShort / d);
        Vector3 em = eMid / nt;
        Vector3 el = eLong / nt;

        //if (shortEdgeIndex != t.shortEdgeIndex || t.nt != nt || t.ns != ns)
        //{
        t.innerParticles.clear();
        t.shortEdgeIndex = shortEdgeIndex;
        t.nt = nt;
        t.ns = ns;
        Vertex inner;

        for (int i = 1; i < nt; ++i)
        {
            Vector3 i1 = mStart + em * i;
            Vector3 i2 = lStart + el * i;
            float l = (i1 - i2).Length();
            int ns = (int)std::floor(l / d);

            Vector3 d = (i1 - i2) / ns;
            
            Vector3 sn1 = (n1 * (nt - i) + n0 * i) / nt; 
            sn1.Normalize(); 
            Vector3 sn2 = (n2 * (nt - i) + n0 * i) / nt; 
            sn2.Normalize();
            
            // 초기화 버그 발생
            //Vector3 n1 = (n1 * (nt - i) + n0 * i) / nt;
            //n1.Normalize();
            //Vector3 n2 = (n2 * (nt - i) + n0 * i) / nt;
            //n2.Normalize();


            for (int j = 1; j < ns; ++j)
            {
                inner.position = i2 + d * j;

                // normal barycentric interpolation
                float s2 = ((p0 - inner.position).Cross(p1 - inner.position)).Length();
                float s1 = ((p2 - inner.position).Cross(p0 - inner.position)).Length();
                float s0 = ((p1 - inner.position).Cross(p2 - inner.position)).Length();
                
                inner.normal = n0 * s0 + n1 * s1 + n2 * s2;
                
                // scan line interpolation
                //inner.normal = (sn2 * (ns - j) + sn1 * j) / ns;

                inner.normal.Normalize();

                t.innerParticles.push_back(inner);
            }
        }
        //}
        //else
        //{
        //    int idx = 0;
        //    for (int i = 1; i < nt; ++i)
        //    {
        //        Vector3 i1 = mStart + em * i;
        //        Vector3 i2 = lStart + el * i;
        //        float l = (i1 - i2).Length();
        //        int ns = (int)std::floor(l / p);

        //        Vector3 d = (i1 - i2) / ns;

        //        for (int j = 1; j < ns; ++j)
        //        {
        //            if(idx < t.inner.size())
        //                t.inner[idx].position = i2 + d * j;
        //            idx++;
        //        }
        //    }
        //}
        m_meshData[0].particleVertices.insert(m_meshData[0].particleVertices.end(), t.innerParticles.begin(), t.innerParticles.end());
    }

    void ExampleApp::initParticle()
    {
        float d = 0.1f;

        for (auto &t :  m_meshData[0].triangles)
        {
            Edge& e0 = m_meshData[0].edges[t.edgeIndices[0]];
            Edge& e1 = m_meshData[0].edges[t.edgeIndices[1]];
            Edge& e2 = m_meshData[0].edges[t.edgeIndices[2]];

            EdgeSampling(e0, d);
            EdgeSampling(e1, d);
            EdgeSampling(e2, d);

            InnerSampling(t, d);
        }
    }

    void ExampleApp::makePointCloudFile()
    {
        // 1. 모든 vertex 모으기 -----------------------------------------------
        std::vector<Vertex> vertices;
        vertices.insert(vertices.end(),
            m_meshData[0].vertices.begin(), m_meshData[0].vertices.end());
        vertices.insert(vertices.end(),
            m_meshData[0].particleVertices.begin(), m_meshData[0].particleVertices.end());

        std::cout << "makePointCloudFile : " << vertices.size() << std::endl;

        const std::size_t vertexCount = vertices.size();
        int MAX_SH_COEFF = 48;

        if (vertexCount == 0)
        {
            std::cout << "No vertices, skip writing point cloud." << std::endl;
            return;
        }

        // 2. 출력 파일 경로 -----------------------------------------------------
        // TODO: 원하는 경로로 변경
        const std::string outPath = "C:/Users/wjdgu/Desktop/meshToPointCloud/point_cloud.ply";

        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs)
        {
            throw std::runtime_error("Failed to open output PLY file: " + outPath);
        }

        // 3. PLY 헤더 작성 ------------------------------------------------------
        ofs << "ply\n";
        ofs << "format binary_little_endian 1.0\n";
        ofs << "element vertex " << vertexCount << "\n";

        // ReadFromFileGS에서 찾는 이름과 정확히 일치해야 함
        ofs << "property float x\n";
        ofs << "property float y\n";
        ofs << "property float z\n";

        ofs << "property float scale_0\n";
        ofs << "property float scale_1\n";
        ofs << "property float scale_2\n";

        ofs << "property float rot_0\n";
        ofs << "property float rot_1\n";
        ofs << "property float rot_2\n";
        ofs << "property float rot_3\n";

        ofs << "property float opacity\n";

        ofs << "property float f_dc_0\n";
        ofs << "property float f_dc_1\n";
        ofs << "property float f_dc_2\n";

        // MAX_SH_COEFF 전체에서 DC 3개 빼고 나머지를 f_rest_k 로
        const int SH_COEFF = MAX_SH_COEFF; // 네가 쓰는 값으로 정의되어 있을 것
        const int REST_COUNT = SH_COEFF - 3;
        for (int i = 0; i < REST_COUNT; ++i)
        {
            ofs << "property float f_rest_" << i << "\n";
        }

        ofs << "end_header\n";

        // 4. vertex 데이터 작성 -------------------------------------------------
        // 기본 값 세팅 (모든 vertex에 동일하게 쓸 값들)
        const float linearScale = 0.1f;
        const float logScale = std::log(linearScale);  // 파일에는 log scale 저장

        // identity quaternion: ReadFromFileGS에서 기본값을 Vector4(1,0,0,0)로 쓰고 있으니 동일하게
        const float rot0 = 1.0f;
        const float rot1 = 0.0f;
        const float rot2 = 0.0f;
        const float rot3 = 0.0f;

        const float opacity = 1.0f;

        // SH: 적당한 값. 여기서는 회색/하양 정도 느낌으로 DC=[0.5,0.5,0.5], 나머지 0
        const float dc0 = 0.5f;
        const float dc1 = 0.5f;
        const float dc2 = 0.5f;

        std::vector<float> restSH(REST_COUNT, 0.0f);

        for (const auto& v : vertices)
        {
            // position
            float x = v.position.x;
            float y = v.position.y;
            float z = v.position.z;

            ofs.write(reinterpret_cast<const char*>(&x), sizeof(float));
            ofs.write(reinterpret_cast<const char*>(&y), sizeof(float));
            ofs.write(reinterpret_cast<const char*>(&z), sizeof(float));

            // scale
            ofs.write(reinterpret_cast<const char*>(&logScale), sizeof(float)); // scale_0
            ofs.write(reinterpret_cast<const char*>(&logScale), sizeof(float)); // scale_1
            ofs.write(reinterpret_cast<const char*>(&logScale), sizeof(float)); // scale_2

            // rotation
            ofs.write(reinterpret_cast<const char*>(&rot0), sizeof(float)); // rot_0
            ofs.write(reinterpret_cast<const char*>(&rot1), sizeof(float)); // rot_1
            ofs.write(reinterpret_cast<const char*>(&rot2), sizeof(float)); // rot_2
            ofs.write(reinterpret_cast<const char*>(&rot3), sizeof(float)); // rot_3

            // opacity
            ofs.write(reinterpret_cast<const char*>(&opacity), sizeof(float));

            // DC SH
            ofs.write(reinterpret_cast<const char*>(&dc0), sizeof(float)); // f_dc_0
            ofs.write(reinterpret_cast<const char*>(&dc1), sizeof(float)); // f_dc_1
            ofs.write(reinterpret_cast<const char*>(&dc2), sizeof(float)); // f_dc_2

            // f_rest_k
            for (int i = 0; i < REST_COUNT; ++i)
            {
                ofs.write(reinterpret_cast<const char*>(&restSH[i]), sizeof(float));
            }
        }

        ofs.close();
        std::cout << "Wrote point cloud PLY: " << outPath << std::endl;
    }

    void ExampleApp::computeConstraintScaling()
    {
        std::vector<Vector3> grad(m_meshData[0].vertices.size(), Vector3(0.0, 0.0, 0.0));
        m_cur_volume = 0.0f;
        float gradSum = 0.0f;
        for (int i = 0; i < m_meshData[0].indices.size(); i = i + 3)
        {
            int index0 = m_meshData[0].indices[i];
            int index1 = m_meshData[0].indices[i + 1];
            int index2 = m_meshData[0].indices[i + 2];

            Vector3 pos0 = m_meshData[0].vertices2[index0].newPosition;
            Vector3 pos1 = m_meshData[0].vertices2[index1].newPosition;
            Vector3 pos2 = m_meshData[0].vertices2[index2].newPosition;

            grad[index0] += (pos1 - pos0).Cross(pos2 - pos0);
            grad[index1] += (pos2 - pos1).Cross(pos0 - pos1);
            grad[index2] += (pos0 - pos2).Cross(pos1 - pos2);

            m_cur_volume += pos0.Cross(pos1).Dot(pos2);
        }

        for (int i = 0; i < grad.size(); ++i)
        {
            gradSum += grad[i].LengthSquared() * m_meshData[0].vertices2[i].invMass;
        }
        


        float c_p1p2p3 = m_cur_volume - m_volume * m_pressure;

        //cout << "Volume constraint : " << c_p1p2p3 << endl;

        m_scaling = -c_p1p2p3 / gradSum;
    }

    void ExampleApp::solveOverpressureConstraint(int index0, int index1, int index2)
    {
        float k = 1.0f;

        Vector3 pos1 = m_meshData[0].vertices2[index0].newPosition;
        Vector3 pos2 = m_meshData[0].vertices2[index1].newPosition;
        Vector3 pos3 = m_meshData[0].vertices2[index2].newPosition;

        float invMass1 = m_meshData[0].vertices2[index0].invMass;
        float invMass2 = m_meshData[0].vertices2[index1].invMass;
        float invMass3 = m_meshData[0].vertices2[index2].invMass;

        Vector3 dp1 = (pos2 - pos1).Cross(pos3 - pos1);
        Vector3 dp2 = (pos3 - pos2).Cross(pos1 - pos2);
        Vector3 dp3 = (pos1 - pos3).Cross(pos2 - pos3);

        dp1 *= invMass1 * m_scaling;
        dp2 *= invMass2 * m_scaling;
        dp3 *= invMass3 * m_scaling;

        pos1 += k * dp1;
        pos2 += k * dp2;
        pos3 += k * dp3;

        //m_meshData[0].vertices2[index0].newPosition = pos1;
        //m_meshData[0].vertices2[index1].newPosition = pos2;
        //m_meshData[0].vertices2[index2].newPosition = pos3;


        //누적 후, 한 번에 업데이트
        m_meshData[0].vertices2[index0].gradPosition += k * dp1;
        m_meshData[0].vertices2[index1].gradPosition += k * dp2;
        m_meshData[0].vertices2[index2].gradPosition += k * dp3;

    }

    void ExampleApp::updateNormal()
    {
        for (auto& v : m_meshData[0].vertices)
        {
            v.normal = Vector3(0.0f, 0.0f, 0.0f);
            v.countNormal = 0;
        }

        for (int i = 0; i < m_meshData[0].indices.size(); i = i + 3)
        {
            int idx1 = m_meshData[0].indices[i];
            int idx2 = m_meshData[0].indices[i + 1];
            int idx3 = m_meshData[0].indices[i + 2];

            Vector3 pos1 = m_meshData[0].vertices[idx1].position;
            Vector3 pos2 = m_meshData[0].vertices[idx2].position;
            Vector3 pos3 = m_meshData[0].vertices[idx3].position;

            Vector3 normal = (pos2 - pos1).Cross(pos3 - pos1);
            normal.Normalize();

            m_meshData[0].vertices[idx1].normal += normal;
            m_meshData[0].vertices[idx2].normal += normal;
            m_meshData[0].vertices[idx3].normal += normal;

            m_meshData[0].vertices[idx1].countNormal += 1;
            m_meshData[0].vertices[idx2].countNormal += 1;
            m_meshData[0].vertices[idx3].countNormal += 1;
        }

        for (auto &v : m_meshData[0].vertices)
        {
            v.normal /= v.countNormal;
            v.normal.Normalize();
        }
    }

    void ExampleApp::updateParticle()
    {
        float d = 0.1f;

        m_meshData[0].particleVertices.clear();
        for (auto& e : m_meshData[0].edges)
            e.visited = false;

        for (auto& t : m_meshData[0].triangles)
        {
            Edge& e0 = m_meshData[0].edges[t.edgeIndices[0]];
            Edge& e1 = m_meshData[0].edges[t.edgeIndices[1]];
            Edge& e2 = m_meshData[0].edges[t.edgeIndices[2]];

            EdgeSampling(e0, d);
            EdgeSampling(e1, d);
            EdgeSampling(e2, d);

            InnerSampling(t, d);
        }

        
    }

    void ExampleApp::integrate(float dt)
    {
        for (auto& meshData : m_meshData) {

            for (int i = 0; i < meshData.vertices.size(); ++i)
            {
                meshData.vertices2[i].velocity = (meshData.vertices2[i].newPosition - meshData.vertices[i].position) / dt;
                meshData.vertices[i].position = meshData.vertices2[i].newPosition;
            }
        }
    }

    void ExampleApp::Wind(int key)
    {
        Vector3 wind = Vector3(-0.5f, -0.25f, -0.5f);
        if (key == 1)
            wind = -wind;
        for (int i = 0; i < m_meshData[0].indices.size(); i = i + 3)
        {
            int idx1 = m_meshData[0].indices[i];
            int idx2 = m_meshData[0].indices[i + 1];
            int idx3 = m_meshData[0].indices[i + 2];

            Vector3 pos1 = m_meshData[0].vertices2[idx1].newPosition;
            Vector3 pos2 = m_meshData[0].vertices2[idx2].newPosition;
            Vector3 pos3 = m_meshData[0].vertices2[idx3].newPosition;

            Vector3 normal = (pos2 - pos1).Cross(pos3 - pos1);
            normal.Normalize();

            if (normal.Dot(-wind) < 0.0f)
            {
                m_meshData[0].vertices2[idx1].velocity -= wind;
                m_meshData[0].vertices2[idx2].velocity -= wind;
                m_meshData[0].vertices2[idx3].velocity -= wind;
            }
        }

    }

    void ExampleApp::ChangeVolume(int key)
    {
        float k = 0.01f;
        if (key == 1)
            k = -k;
        
        m_pressure += k;

        cout << "Voluem : " << m_pressure << endl;
    }

    void ExampleApp::ChangeView(int key)
    {
        float k = 0.05f;
        if (key == 87)
            m_viewTranslation.z -= k;
        else if (key == 65)
            m_viewTranslation.x += k;
        else if (key == 83)
            m_viewTranslation.z += k;
        else if (key == 68)
            m_viewTranslation.x -= k;
        else if (key == 107)
            m_viewTranslation.y -= k;
        else if (key == 109)
            m_viewTranslation.y += k;
        else if (key == 69)
            m_viewRot.y += k;
        else if (key == 81)
            m_viewRot.y -= k;
    }

    void ExampleApp::Update(float dt) {

        using namespace DirectX;

        applyExtForces(dt);

        for (auto& meshData : m_meshData)
        {
            
            for (int i = 0; i < 5; ++i)
            {
                computeConstraintScaling();

                for (auto& edge : meshData.edges)
                {
                    solveDistanceConstraint(edge.index0, edge.index1, edge.restLength);
                }

                for (auto& vertex : meshData.vertices2)
                {
                    vertex.bendingForce = 0.0f;
                    vertex.gradPosition = Vector3(0.0f, 0.0f, 0.0f);
                }

                for (auto& rec : meshData.recs)
                {
                    //solveDihedralConstraint(rec.index0, rec.index1, rec.index2, rec.index3, rec.restAngle);
                }

                for (int i = 0; i < meshData.indices.size(); i = i + 3)
                {
                    int index0 = m_meshData[0].indices[i];
                    int index1 = m_meshData[0].indices[i + 1];
                    int index2 = m_meshData[0].indices[i + 2];
                    solveOverpressureConstraint(index0, index1, index2);
                }

                //누적 후, 한 번에 업데이트
                for (auto& vertex : meshData.vertices2)
                {
                    vertex.newPosition += vertex.gradPosition;
                }
            }
        }

        integrate(dt);
        updateNormal();
        updateParticle();

        if (m_meshes[0]) {
            m_meshData[0].totalVertices.clear();
            m_meshData[0].totalVertices.insert(m_meshData[0].totalVertices.end(), m_meshData[0].vertices.begin(), m_meshData[0].vertices.end());
            m_meshData[0].totalVertices.insert(m_meshData[0].totalVertices.end(), m_meshData[0].particleVertices.begin(), m_meshData[0].particleVertices.end());

            //cout << m_meshData[0].totalVertices.size() << endl;
            m_meshes[0]->m_indexCount = UINT(m_meshData[0].totalVertices.size());
            AppBase::UpdateVertexBuffer(m_meshData[0].totalVertices,
                m_meshes[0]->vertexBuffer);
        }

        // 모델 변환
        m_BasicVertexConstantBufferData.model =
            Matrix::CreateScale(m_modelScaling) *
            Matrix::CreateRotationY(m_modelRotation.y) *
            Matrix::CreateRotationX(m_modelRotation.x) *
            Matrix::CreateRotationZ(m_modelRotation.z) *
            Matrix::CreateTranslation(m_modelTranslation);
        m_BasicVertexConstantBufferData.model =
            m_BasicVertexConstantBufferData.model.Transpose();
        m_BasicGeometryConstantBufferData.model = m_BasicVertexConstantBufferData.model;


        m_BasicVertexConstantBufferData.invTranspose =
            m_BasicVertexConstantBufferData.model;
        m_BasicVertexConstantBufferData.invTranspose.Translation(Vector3(0.0f));
        m_BasicVertexConstantBufferData.invTranspose =
            m_BasicVertexConstantBufferData.invTranspose.Transpose().Invert();
        m_BasicGeometryConstantBufferData.invTranspose = m_BasicVertexConstantBufferData.invTranspose;

        // 시점 변환
        m_BasicVertexConstantBufferData.view =
            Matrix::CreateRotationY(m_viewRot.y) *
            Matrix::CreateRotationZ(m_viewRot.z) *
            Matrix::CreateRotationX(m_viewRot.x) *
            Matrix::CreateTranslation(m_viewTranslation);

        Matrix viewInv = m_BasicVertexConstantBufferData.view.Invert();
        m_BasicGeometryConstantBufferData.gCamRightWS = Vector3(viewInv._11, viewInv._12, viewInv._13);
        m_BasicGeometryConstantBufferData.gCamUpWS = Vector3(viewInv._21, viewInv._22, viewInv._23);

        m_BasicPixelConstantBufferData.eyeWorld = Vector3::Transform(
            Vector3(0.0f), m_BasicVertexConstantBufferData.view.Invert());

        m_BasicVertexConstantBufferData.view =
            m_BasicVertexConstantBufferData.view.Transpose();
        m_BasicGeometryConstantBufferData.view = m_BasicVertexConstantBufferData.view;

        // 프로젝션
        const float aspect = AppBase::GetAspectRatio();
        if (m_usePerspectiveProjection) {
            m_BasicVertexConstantBufferData.projection = XMMatrixPerspectiveFovLH(
                XMConvertToRadians(m_projFovAngleY), aspect, m_nearZ, m_farZ);
        }
        else {
            m_BasicVertexConstantBufferData.projection =
                XMMatrixOrthographicOffCenterLH(-aspect, aspect, -1.0f, 1.0f,
                    m_nearZ, m_farZ);
        }
        m_BasicVertexConstantBufferData.projection =
            m_BasicVertexConstantBufferData.projection.Transpose();
        m_BasicGeometryConstantBufferData.projection = m_BasicVertexConstantBufferData.projection;
        m_BasicGeometryConstantBufferData.scaling = m_gaussian_scaling;

        if (m_meshes[0]) {
            AppBase::UpdateBuffer(m_BasicVertexConstantBufferData,
                m_meshes[0]->vertexConstantBuffer);
        }

        if (m_meshes[0]) {
            AppBase::UpdateBuffer(m_BasicGeometryConstantBufferData,
                m_meshes[0]->geometryConstantBuffer);
        }

        m_BasicPixelConstantBufferData.material.diffuse =
            Vector3(m_materialDiffuse);
        m_BasicPixelConstantBufferData.material.specular =
            Vector3(m_materialSpecular);

        for (int i = 0; i < MAX_LIGHTS; i++) {
            if (i != m_lightType) {
                m_BasicPixelConstantBufferData.lights[i].strength *= 0.0f;
            }
            else {
                m_BasicPixelConstantBufferData.lights[i] = m_lightFromGUI;
            }
        }

        if (m_meshes[0]) {
            AppBase::UpdateBuffer(m_BasicPixelConstantBufferData,
                m_meshes[0]->pixelConstantBuffer);
        }


        // 노멀 벡터 그리기
        if (m_drawNormals && m_drawNormalsDirtyFlag) {

            AppBase::UpdateBuffer(m_normalVertexConstantBufferData,
                m_normalLines->vertexConstantBuffer);

            m_drawNormalsDirtyFlag = false;
        }

        // 큐브매핑을 위한 ConstantBuffers
        m_BasicVertexConstantBufferData.model = Matrix();
        // Transpose()도 생략 가능

        AppBase::UpdateBuffer(m_BasicVertexConstantBufferData,
            m_cubeMapping.cubeMesh->vertexConstantBuffer);

    }

    void ExampleApp::Render() {

        // RS: Rasterizer stage
        // OM: Output-Merger stage
        // VS: Vertex Shader
        // PS: Pixel Shader
        // IA: Input-Assembler stage

        SetViewport();


        UINT stride = sizeof(Vertex);
        UINT offset = 0;


        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
        m_context->ClearDepthStencilView(m_depthStencilView.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f, 0);


        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
            m_depthStencilView.Get());
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);


        // 알파 블렌딩
        float blendFactor[4] = { 1,1,1,1 };
        UINT sampleMask = 0xffffffff;
        m_context->OMSetBlendState(m_blendState.Get(), blendFactor, sampleMask);


        // 큐브매핑
        m_context->IASetInputLayout(m_cubeMapping.inputLayout.Get());
        m_context->IASetVertexBuffers(
            0, 1, m_cubeMapping.cubeMesh->vertexBuffer.GetAddressOf(), &stride,
            &offset);
        m_context->IASetIndexBuffer(m_cubeMapping.cubeMesh->indexBuffer.Get(),
            DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_context->GSSetShader(nullptr, nullptr, 0);
        m_context->VSSetShader(m_cubeMapping.vertexShader.Get(), 0, 0);
        m_context->VSSetConstantBuffers(
            0, 1, m_cubeMapping.cubeMesh->vertexConstantBuffer.GetAddressOf());

        ID3D11ShaderResourceView* views[1] = {
    m_cubeMapping.cubemapResourceView.Get() };
        m_context->PSSetShaderResources(0, 1, views);
        m_context->PSSetShader(m_cubeMapping.pixelShader.Get(), 0, 0);
        m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

        m_context->DrawIndexed(m_cubeMapping.cubeMesh->m_indexCount, 0, 0);


        // 물체들
        m_context->VSSetShader(m_basicVertexShader.Get(), 0, 0);
        m_context->GSSetShader(m_basicGeometryShader.Get(), 0, 0);
        m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
        m_context->PSSetShader(m_basicPixelShader.Get(), 0, 0);

        if (m_drawAsWire) {
            m_context->RSSetState(m_wireRasterizerSate.Get());
        }
        else {
            m_context->RSSetState(m_solidRasterizerSate.Get());
        }

        for (const auto& mesh : m_meshes) {
            m_context->VSSetConstantBuffers(
                0, 1, mesh->vertexConstantBuffer.GetAddressOf());

            m_context->GSSetConstantBuffers(
                0, 1, mesh->geometryConstantBuffer.GetAddressOf());

            ID3D11ShaderResourceView* views[2] = {
                m_textureResourceView.Get(), m_cubeMapping.cubemapResourceView.Get()};
            m_context->PSSetShaderResources(0, 2, views);

            m_context->PSSetConstantBuffers(
                0, 1, mesh->pixelConstantBuffer.GetAddressOf());

            m_context->IASetInputLayout(m_basicInputLayout.Get());
            m_context->IASetVertexBuffers(0, 1, mesh->vertexBuffer.GetAddressOf(),
                &stride, &offset);
            m_context->IASetIndexBuffer(mesh->indexBuffer.Get(),
                DXGI_FORMAT_R32_UINT, 0);
            m_context->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

            // index 없이 point만 Draw
            m_context->Draw(mesh->m_indexCount, 0);
        }

        // 노멀 벡터 그리기
        if (m_drawNormals) {
            m_context->GSSetShader(nullptr, nullptr, 0);
            m_context->VSSetShader(m_normalVertexShader.Get(), 0, 0);

            ID3D11Buffer* pptr[2] = { m_meshes[0]->vertexConstantBuffer.Get(),
                                     m_normalLines->vertexConstantBuffer.Get() };
            m_context->VSSetConstantBuffers(0, 2, pptr);

            m_context->PSSetShader(m_normalPixelShader.Get(), 0, 0);
            // m_context->IASetInputLayout(m_basicInputLayout.Get());
            m_context->IASetVertexBuffers(
                0, 1, m_normalLines->vertexBuffer.GetAddressOf(), &stride, &offset);
            m_context->IASetIndexBuffer(m_normalLines->indexBuffer.Get(),
                DXGI_FORMAT_R32_UINT, 0);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            m_context->DrawIndexed(m_normalLines->m_indexCount, 0, 0);
        }


    }

    void ExampleApp::UpdateGUI() {

        ImGui::Checkbox("Use Texture", &m_BasicPixelConstantBufferData.useTexture);
        ImGui::Checkbox("Wireframe", &m_drawAsWire);
        ImGui::Checkbox("Draw Normals", &m_drawNormals);
        ImGui::Checkbox("View Rotation Y", &m_viewRotationY);
        if (m_viewRotationY)
        {
            m_viewRot.y -= 0.001f;
        }
        
        if (ImGui::SliderFloat("Normal scale",
            &m_normalVertexConstantBufferData.scale, 0.0f,
            1.0f)) {
            m_drawNormalsDirtyFlag = true;
        }
        ImGui::SliderFloat("Gaussian scale", &m_gaussian_scaling, 0.0f, 10.0f);

        ImGui::SliderFloat3("m_modelTranslation", &m_modelTranslation.x, -8.0f,
            8.0f);

        ImGui::SliderFloat3("m_modelRotation", &m_modelRotation.x, -3.14f, 3.14f);
        ImGui::SliderFloat3("m_modelScaling", &m_modelScaling.x, 0.1f, 2.0f);
        ImGui::SliderFloat3("m_viewTranslation", &m_viewTranslation.x, -8.0f,
            8.0f);
        ImGui::SliderFloat3("m_viewRot", &m_viewRot.x, -3.14f, 3.14f);

        ImGui::SliderFloat("m_pressure", &m_pressure, 0.0f, 3.0f);

        ImGui::SliderFloat("Material Shininess",
            &m_BasicPixelConstantBufferData.material.shininess, 1.0f,
            256.0f);

        if (ImGui::RadioButton("Directional Light", m_lightType == 0)) {
            m_lightType = 0;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Point Light", m_lightType == 1)) {
            m_lightType = 1;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Spot Light", m_lightType == 2)) {
            m_lightType = 2;
        }

        ImGui::SliderFloat("Material Diffuse", &m_materialDiffuse, 0.0f, 1.0f);
        ImGui::SliderFloat("Material Specular", &m_materialSpecular, 0.0f, 1.0f);

        ImGui::SliderFloat3("Light Position", &m_lightFromGUI.position.x, -5.0f,
            5.0f);

        ImGui::SliderFloat("Light fallOffStart", &m_lightFromGUI.fallOffStart, 0.0f,
            5.0f);

        ImGui::SliderFloat("Light fallOffEnd", &m_lightFromGUI.fallOffEnd, 0.0f,
            10.0f);

        ImGui::SliderFloat("Light spotPower", &m_lightFromGUI.spotPower, 1.0f,
            512.0f);


    }


   
} 
