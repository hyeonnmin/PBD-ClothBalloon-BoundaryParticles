#include "GeometryGenerator.h"

#include "ModelLoader.h"

namespace jhm {

    using namespace std;
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    int GeometryGenerator::AddEdge(UINT index0, UINT index1, MeshData& mesh, bool crossEdge)
    {
        bool check = false;
        int idx = 0;
        UINT idx0 = min(index0, index1);
        UINT idx1 = max(index0, index1);
        vector<Vertex>& vertices = mesh.vertices;
        vector<Edge>& edges = mesh.edges;

        for (auto& e : edges)
        {
            if (e.index0 == idx0 && e.index1 == idx1)
            {
                
                check = true;
                break;
            }
            ++idx;
        }

        if (check == false)
            edges.push_back({ idx0, idx1, (vertices[idx0].position - vertices[idx1].position).Length() });

        return idx;
    }

    void GeometryGenerator::AddRecEdge(UINT index0, UINT index1, UINT index2, UINT index3, MeshData& mesh) {

        vector<Vertex>& vertices = mesh.vertices;
        vector<Rectangle>& recs = mesh.recs;

        Vector3 n1 = (vertices[index0].position - vertices[index1].position).Cross(vertices[index2].position - vertices[index1].position);
        Vector3 n2 = (vertices[index3].position - vertices[index0].position).Cross(vertices[index2].position - vertices[index0].position);
        n1.Normalize();
        n2.Normalize();

        float dot = n1.Dot(n2);
        if (dot < -1.0) {
            dot = -1.0;
        }
        if (dot > 1.0) {
            dot = 1.0;
        }

        Rectangle rec = { index1, index3, index2, index0, acos(dot) };
        recs.push_back(rec);


        //AddEdge(index0, index1, mesh, false);
        //AddEdge(index1, index2, mesh, false);
        //AddEdge(index2, index3, mesh, false);
        //AddEdge(index3, index0, mesh, false);
        //AddEdge(index1, index3, mesh, false);
        //AddEdge(index2, index0, mesh, false);
    }

    bool GeometryGenerator::FindPair(UINT idx0, UINT idx1, UINT idx2, UINT idx3, UINT idx4, UINT idx5, MeshData& mesh) {

        bool check = true;
        if (idx0 == idx3 && idx1 == idx5)
        {
            AddRecEdge(idx0, idx4, idx5, idx2, mesh);
        }
        else if (idx0 == idx5 && idx1 == idx4)
        {
            AddRecEdge(idx0, idx3, idx4, idx2, mesh);
        }
        else if (idx0 == idx4 && idx1 == idx3)
        {
            AddRecEdge(idx1, idx2, idx4, idx5, mesh);

        }
        else if (idx1 == idx3 && idx2 == idx5)
        {
            AddRecEdge(idx1, idx4, idx5, idx0, mesh);
        }
        else if (idx1 == idx5 && idx2 == idx4)
        {
            AddRecEdge(idx1, idx3, idx4, idx0, mesh);
        }
        else if (idx1 == idx4 && idx2 == idx3)
        {
            AddRecEdge(idx4, idx5, idx3, idx0, mesh);
        }
        else if (idx2 == idx3 && idx0 == idx5)
        {
            AddRecEdge(idx2, idx4, idx5, idx1, mesh);
        }
        else if (idx2 == idx5 && idx0 == idx4)
        {
            AddRecEdge(idx2, idx3, idx0, idx1, mesh);
        }
        else if (idx2 == idx4 && idx0 == idx3)
        {
            AddRecEdge(idx2, idx5, idx3, idx1, mesh);
        }
        else
            check = false;
        return check;
    }

    vector<MeshData> GeometryGenerator::ReadFromFile(std::string basePath,
        std::string filename) {

        using namespace DirectX;

        ModelLoader modelLoader;
        modelLoader.Load(basePath, filename);
        vector<MeshData>& meshes = modelLoader.meshes;

        // Normalize vertices
        Vector3 vmin(1000, 1000, 1000);
        Vector3 vmax(-1000, -1000, -1000);
        for (auto& mesh : meshes) {
            for (auto& v : mesh.vertices) {

                vmin.x = XMMin(vmin.x, v.position.x);
                vmin.y = XMMin(vmin.y, v.position.y);
                vmin.z = XMMin(vmin.z, v.position.z);
                vmax.x = XMMax(vmax.x, v.position.x);
                vmax.y = XMMax(vmax.y, v.position.y);
                vmax.z = XMMax(vmax.z, v.position.z);
            }
        }

        float dx = vmax.x - vmin.x, dy = vmax.y - vmin.y, dz = vmax.z - vmin.z;
        float dl = XMMax(XMMax(dx, dy), dz);
        float cx = (vmax.x + vmin.x) * 0.5f, cy = (vmax.y + vmin.y) * 0.5f,
            cz = (vmax.z + vmin.z) * 0.5f;

        
        dl = 1.0f;
        for (auto& mesh : meshes) {
            for (auto& v : mesh.vertices) {
                v.position.x = (v.position.x - cx) / dl;
                v.position.y = (v.position.y - cy) / dl;
                v.position.z = (v.position.z - cz) / dl;
            }
        }

        vector<Triangle>& triangles = meshes[0].triangles;
        Triangle t;
        for (int i = 0; i < meshes[0].indices.size(); i = i + 3)
        {
            int index0 = meshes[0].indices[i];
            int index1 = meshes[0].indices[i + 1];
            int index2 = meshes[0].indices[i + 2];

            t.vertexIndices[0] = index0;
            t.vertexIndices[1] = index1;
            t.vertexIndices[2] = index2;

            t.edgeIndices[0] = AddEdge(index0, index1, meshes[0], false);
            t.edgeIndices[1] = AddEdge(index1, index2, meshes[0], false);
            t.edgeIndices[2] = AddEdge(index2, index0, meshes[0], false);

            triangles.push_back(t);
        }

        cout << meshes[0].vertices.size() << endl;
        cout << meshes[0].indices.size() << endl;

        return meshes;
    }

    

    MeshData GeometryGenerator::MakeSphere(const float radius, const int numSlices,
        const int numStacks) {

        // 참고: OpenGL Sphere
        // http://www.songho.ca/opengl/gl_sphere.html
        // Texture 좌표계때문에 (numSlices + 1) 개의 버텍스 사용 (마지막에 닫아주는
        // 버텍스가 중복) Stack은 y 위쪽 방향으로 쌓아가는 방식

        const float dTheta = -XM_2PI / float(numSlices);
        const float dPhi = -XM_PI / float(numStacks);

        MeshData meshData;

        vector<Vertex>& vertices = meshData.vertices;

        for (int j = 0; j <= numStacks; j++) {

            // 스택에 쌓일 수록 시작점을 x-y 평면에서 회전 시켜서 위로 올리는 구조
            Vector3 stackStartPoint = Vector3::Transform(
                Vector3(0.0f, -radius, 0.0f), Matrix::CreateRotationZ(dPhi * j));

            // 원 꼭짓점
            if (j == 0 || j == numStacks)
            {
                Vertex v;
                Vertex2 v2;

                v.position = stackStartPoint;
                v.normal = v.position;
                v.normal.Normalize();
                
                v2.invMass = 1.0f;
                v2.newPosition = Vector3(0.0, 0.0, 0.0);
                v2.velocity = Vector3(0.0, 0.0, 0.0);

                vertices.push_back(v);
                meshData.vertices2.push_back(v2);
            }
            else
            {
                for (int i = 0; i < numSlices; i++) {
                    Vertex v;
                    Vertex2 v2;

                    // 시작점을 x-z 평면에서 회전시키면서 원을 만드는 구조
                    v.position = Vector3::Transform(
                        stackStartPoint, Matrix::CreateRotationY(dTheta * float(i)));

                    v.normal = v.position; // 원점이 구의 중심
                    v.normal.Normalize();
                    v.texcoord =
                        Vector2(float(i) / numSlices, 1.0f - float(j) / numStacks);

                    v2.invMass = 1.0f;
                    v2.newPosition = Vector3(0.0, 0.0, 0.0);
                    v2.velocity = Vector3(0.0, 0.0, 0.0);

                    vertices.push_back(v);
                    meshData.vertices2.push_back(v2);
                }
            }
        }

        cout << "vertices size : " << vertices.size() << endl;

        vector<uint32_t>& indices = meshData.indices;
        vector<Triangle>& triangles = meshData.triangles;
        
        int offset = 0;
        Triangle t;
        for (int j = 0; j < numStacks; j++) {

            if (j == 0)
            {
                for (int i = 0; i < numSlices; i++)
                {
                    int index0 = offset;
                    int index1 = offset + 1 + i;
                    int index2 = offset + 1 + (1 + i) % numSlices;

                    indices.push_back(index0);
                    indices.push_back(index1);
                    indices.push_back(index2);

                    t.vertexIndices[0] = index0;
                    t.vertexIndices[1] = index1;
                    t.vertexIndices[2] = index2;

                    t.edgeIndices[0] = AddEdge(index0, index1, meshData, false);
                    t.edgeIndices[1] = AddEdge(index1, index2, meshData, false);
                    t.edgeIndices[2] = AddEdge(index2, index0, meshData, false);

                    triangles.push_back(t);
                }
                ++offset;
            }
            else if (j == numStacks - 1)
            {
                for (int i = 0; i < numSlices; i++)
                {
                    int index0 = offset + i;
                    int index1 = offset + numSlices;
                    int index2 = offset + (i + 1) % numSlices;

                    indices.push_back(index0);
                    indices.push_back(index1);
                    indices.push_back(index2);

                    t.vertexIndices[0] = index0;
                    t.vertexIndices[1] = index1;
                    t.vertexIndices[2] = index2;

                    t.edgeIndices[0] = AddEdge(index0, index1, meshData, false);
                    t.edgeIndices[1] = AddEdge(index1, index2, meshData, false);
                    t.edgeIndices[2] = AddEdge(index2, index0, meshData, false);

                    triangles.push_back(t);
                }
            }
            else
            {
                for (int i = 0; i < numSlices; i++) {

                    int index0 = offset + i;
                    int index1 = offset + i + numSlices;
                    int index2 = offset + (i + 1) % numSlices + numSlices;

                    indices.push_back(index0);
                    indices.push_back(index1);
                    indices.push_back(index2);

                    t.vertexIndices[0] = index0;
                    t.vertexIndices[1] = index1;
                    t.vertexIndices[2] = index2;

                    t.edgeIndices[0] = AddEdge(index0, index1, meshData, false);
                    t.edgeIndices[1] = AddEdge(index1, index2, meshData, false);
                    t.edgeIndices[2] = AddEdge(index2, index0, meshData, false);

                    triangles.push_back(t);

                    index0 = offset + i;
                    index1 = offset + (1 + i) % numSlices + numSlices;
                    index2 = offset + (1 + i) % numSlices;

                    indices.push_back(index0);
                    indices.push_back(index1);
                    indices.push_back(index2);

                    t.vertexIndices[0] = index0;
                    t.vertexIndices[1] = index1;
                    t.vertexIndices[2] = index2;

                    t.edgeIndices[0] = AddEdge(index0, index1, meshData, false);
                    t.edgeIndices[1] = AddEdge(index1, index2, meshData, false);
                    t.edgeIndices[2] = AddEdge(index2, index0, meshData, false);

                    triangles.push_back(t);
                }
                offset += numSlices;
            }
        }

        cout << "indices sizes : " << indices.size() << endl;

        cout << "edges size : " << meshData.edges.size() << endl;

        return meshData;
    }

    MeshData GeometryGenerator::MakeBox(const float scale) {

        vector<Vector3> positions;
        vector<Vector3> colors;
        vector<Vector3> normals;
        vector<Vector2> texcoords; // 텍스춰 좌표
        
        positions.resize(8);
        colors.resize(8);
        normals.resize(8);
        texcoords.resize(8);

        // 0~7번 꼭짓점 정의 (front: z=-1, back: z=1)
        positions[0] = Vector3(-1.0f, -1.0f, -1.0f) * scale; // 좌하단 앞
        positions[1] = Vector3(-1.0f, 1.0f, -1.0f) * scale; // 좌상단 앞
        positions[2] = Vector3(1.0f, 1.0f, -1.0f) * scale; // 우상단 앞
        positions[3] = Vector3(1.0f, -1.0f, -1.0f) * scale; // 우하단 앞

        positions[4] = Vector3(-1.0f, -1.0f, 1.0f) * scale; // 좌하단 뒤
        positions[5] = Vector3(-1.0f, 1.0f, 1.0f) * scale; // 좌상단 뒤
        positions[6] = Vector3(1.0f, 1.0f, 1.0f) * scale; // 우상단 뒤
        positions[7] = Vector3(1.0f, -1.0f, 1.0f) * scale; // 우하단 뒤

        float invSqrt3 = 1.0f / sqrtf(3.0f);

        normals[0] = Vector3(-1.0f, -1.0f, -1.0f) * invSqrt3;
        normals[1] = Vector3(-1.0f, 1.0f, -1.0f) * invSqrt3;
        normals[2] = Vector3(1.0f, 1.0f, -1.0f) * invSqrt3;
        normals[3] = Vector3(1.0f, -1.0f, -1.0f) * invSqrt3;

        normals[4] = Vector3(-1.0f, -1.0f, 1.0f) * invSqrt3;
        normals[5] = Vector3(-1.0f, 1.0f, 1.0f) * invSqrt3;
        normals[6] = Vector3(1.0f, 1.0f, 1.0f) * invSqrt3;
        normals[7] = Vector3(1.0f, -1.0f, 1.0f) * invSqrt3;

        // -------- colors : 인접 면 색 3개 평균 --------
        // 윗면: red(1,0,0)
        // 아랫면: green(0,1,0)
        // 앞면: blue(0,0,1)
        // 뒷면: cyan(0,1,1)
        // 왼쪽: yellow(1,1,0)
        // 오른쪽: magenta(1,0,1)

        // 0: bottom + front + left
        colors[0] = Vector3(
            (0.0f + 0.0f + 1.0f) / 3.0f,
            (1.0f + 0.0f + 1.0f) / 3.0f,
            (0.0f + 1.0f + 0.0f) / 3.0f); 

        // 1: top + front + left
        colors[1] = Vector3(
            (1.0f + 0.0f + 1.0f) / 3.0f,
            (0.0f + 0.0f + 1.0f) / 3.0f,
            (0.0f + 1.0f + 0.0f) / 3.0f); 

        // 2: top + front + right
        colors[2] = Vector3(
            (1.0f + 0.0f + 1.0f) / 3.0f,
            (0.0f + 0.0f + 0.0f) / 3.0f,
            (0.0f + 1.0f + 1.0f) / 3.0f);

        // 3: bottom + front + right
        colors[3] = Vector3(
            (0.0f + 0.0f + 1.0f) / 3.0f,
            (1.0f + 0.0f + 0.0f) / 3.0f,
            (0.0f + 1.0f + 1.0f) / 3.0f); 

        // 4: bottom + back + left
        colors[4] = Vector3(
            (0.0f + 0.0f + 1.0f) / 3.0f,
            (1.0f + 1.0f + 1.0f) / 3.0f,
            (0.0f + 1.0f + 0.0f) / 3.0f);

        // 5: top + back + left
        colors[5] = Vector3(
            (1.0f + 0.0f + 1.0f) / 3.0f,
            (0.0f + 1.0f + 1.0f) / 3.0f,
            (0.0f + 1.0f + 0.0f) / 3.0f); 

        // 6: top + back + right
        colors[6] = Vector3(
            (1.0f + 0.0f + 1.0f) / 3.0f,
            (0.0f + 1.0f + 0.0f) / 3.0f,
            (0.0f + 1.0f + 1.0f) / 3.0f); 

        // 7: bottom + back + right
        colors[7] = Vector3(
            (0.0f + 0.0f + 1.0f) / 3.0f,
            (1.0f + 1.0f + 0.0f) / 3.0f,
            (0.0f + 1.0f + 1.0f) / 3.0f); 

        // -------- texcoords : position 기반 간단 매핑 --------
        // (이제 한 꼭짓점이 여러 면에서 공유되기 때문에,
        // 각 면마다 (0,0)-(1,1) UV 쓰는 건 불가능해짐)
        // 여기서는 x,z기준으로 [0,1] 매핑 예시:
        for (int i = 0; i < 8; ++i)
        {
            Vector3 p = positions[i] / scale; // -1~1 범위로 정규화
            float u = (p.x + 1.0f) * 0.5f;
            float v = (p.z + 1.0f) * 0.5f;
            texcoords[i] = Vector2(u, v);
        }

        MeshData meshData;

        for (size_t i = 0; i < positions.size(); i++) {
            Vertex v;
            v.position = positions[i];
            v.normal = normals[i];
            v.texcoord = texcoords[i];
            meshData.vertices.push_back(v);

            Vertex2 v2;
            v2.invMass = 1.0f;
            v2.newPosition = Vector3(0.0, 0.0, 0.0);
            v2.velocity = Vector3(0.0, 0.0, 0.0);
            meshData.vertices2.push_back(v2);
        }

        meshData.indices = {
            // 앞면 (z = -1)
            0, 1, 2,
            0, 2, 3,

            // 뒷면 (z = 1)
            4, 6, 5,
            4, 7, 6,

            // 왼쪽면 (x = -1)
            0, 4, 5,
            0, 5, 1,

            // 오른쪽면 (x = 1)
            3, 2, 6,
            3, 6, 7,

            // 윗면 (y = 1)
            1, 5, 6,
            1, 6, 2,

            // 아랫면 (y = -1)
            0, 3, 7,
            0, 7, 4
        };


        
        Triangle t;
        for (int i = 0; i < meshData.indices.size(); i = i + 3)
        {
            int index0 = meshData.indices[i];
            int index1 = meshData.indices[i + 1];
            int index2 = meshData.indices[i + 2];

            t.vertexIndices[0] = index0;
            t.vertexIndices[1] = index1;
            t.vertexIndices[2] = index2;

            t.edgeIndices[0] = AddEdge(index0, index1, meshData, false);
            t.edgeIndices[1] = AddEdge(index1, index2, meshData, false);
            t.edgeIndices[2] = AddEdge(index2, index0, meshData, false);

            meshData.triangles.push_back(t);
        }


        return meshData;
    }
}

   
