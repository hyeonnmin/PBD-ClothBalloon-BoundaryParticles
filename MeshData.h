#pragma once

#include <directxtk/SimpleMath.h>
#include <string>
#include <vector>

#include "Vertex.h"
#include "Vertex2.h"
#include "Edge.h"
#include "Rectangle.h"
#include "Triangle.h"

namespace jhm {

    using std::vector;
 
    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<Vertex2> vertices2;
        std::vector<Vertex> particleVertices;
        std::vector<Vertex> totalVertices;
        std::vector<Triangle> triangles;
        std::vector<Edge> edges;
        std::vector<Rectangle> recs;
        std::vector<uint32_t> indices;
        std::string textureFilename;
    };

}
