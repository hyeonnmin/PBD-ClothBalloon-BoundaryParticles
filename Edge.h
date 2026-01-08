#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

namespace jhm {

    struct Edge {
        UINT index0;
        UINT index1;
        float restLength;
        bool visited = false;
        int n = -1;
        std::vector<Vertex> edgeParticles;
    };

}