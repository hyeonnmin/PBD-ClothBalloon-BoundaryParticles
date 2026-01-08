#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

namespace jhm {

    struct Triangle {
        UINT vertexIndices[3];
        UINT edgeIndices[3];
        std::vector<Vertex> innerParticles;
        UINT shortEdgeIndex;
        UINT nt;
        UINT ns;
    };
}
