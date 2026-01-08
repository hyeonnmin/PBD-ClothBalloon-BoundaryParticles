#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>
#include <string>

#include "Vertex.h"
#include "MeshData.h"
#include "Edge.h"
#include "Face.h"

namespace jhm {

    class GeometryGenerator {
    public:
        static vector<MeshData> ReadFromFile(std::string basePath,
            std::string filename);
        static MeshData MakeBox(const float scale = 1.0f);
        static MeshData MakeSphere(const float radius, const int numSlices,
            const int numStacks);

        static int AddEdge(UINT index0, UINT index1, MeshData& mesh, bool crossEdge);
        static void AddRecEdge(UINT index0, UINT index1, UINT index2, UINT index3, MeshData& mesh);
        static bool FindPair(UINT idx0, UINT idx1, UINT idx2, UINT idx3, UINT idx4, UINT idx5, MeshData& mesh);
    };
}