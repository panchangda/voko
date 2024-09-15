//
// Created by pcd on 2024/9/15.
//

#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <array>
#include "glm/fwd.hpp"


namespace voko {
    enum Side
    {
        LEFT   = 0,
        RIGHT  = 1,
        TOP    = 2,
        BOTTOM = 3,
        BACK   = 4,
        FRONT  = 5
    };

    class Frustum {
    public:
        static std::array<glm::vec4, 8> calculate_corners(const glm::mat4& viewProjMatrix);

    public:
        void update(const glm::mat4 &Matrix);

        bool check_sphere(glm::vec3 pos, float radius);
        const std::array<glm::vec4, 6> &get_planes() const;

        const std::array<glm::vec4, 6> &get_corners()const;
        std::array<glm::vec4, 6> planes;
        std::array<glm::vec4, 8> corners;
    };

}



#endif //FRUSTUM_H
