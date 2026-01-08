#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

namespace jhm {

    using DirectX::SimpleMath::Vector2;
    using DirectX::SimpleMath::Vector3;

    struct Vertex2 {
        Vector3 newPosition;
        Vector3 gradPosition;
        Vector3 velocity;
        float invMass;
        float bendingForce;
    };

}