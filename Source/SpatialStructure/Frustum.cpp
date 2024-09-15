//
// Created by pcd on 2024/9/15.
//

#include "Frustum.h"
#include "glm/matrix.hpp"

namespace voko {
    std::array<glm::vec4, 8> Frustum::calculate_corners(const glm::mat4& viewProjMatrix)
    {
        // 裁剪空间的 8 个点
        static glm::vec4 frustumCornersClipSpace[8] = {
            {-1.0f,  1.0f, -1.0f, 1.0f}, // 近裁剪面左上
            { 1.0f,  1.0f, -1.0f, 1.0f}, // 近裁剪面右上
            {-1.0f, -1.0f, -1.0f, 1.0f}, // 近裁剪面左下
            { 1.0f, -1.0f, -1.0f, 1.0f}, // 近裁剪面右下
            {-1.0f,  1.0f,  1.0f, 1.0f}, // 远裁剪面左上
            { 1.0f,  1.0f,  1.0f, 1.0f}, // 远裁剪面右上
            {-1.0f, -1.0f,  1.0f, 1.0f}, // 远裁剪面左下
            { 1.0f, -1.0f,  1.0f, 1.0f}, // 远裁剪面右下
        };

        // 计算视图投影矩阵的逆矩阵
        glm::mat4 invViewProj = glm::inverse(viewProjMatrix);

        std::array<glm::vec4, 8> frustumCornersWorldSpace;

        // 将裁剪空间的点转换到世界空间
        for (int i = 0; i < 8; i++) {
            // 乘以逆矩阵
            glm::vec4 worldSpacePoint = invViewProj * frustumCornersClipSpace[i];
            // 齐次坐标除以 w 分量，转换为三维坐标
            frustumCornersWorldSpace[i] = worldSpacePoint / worldSpacePoint.w;
        }

        return frustumCornersWorldSpace;
    }

    void Frustum::update(const glm::mat4 &matrix)
    {
        planes[LEFT].x = matrix[0].w + matrix[0].x;
        planes[LEFT].y = matrix[1].w + matrix[1].x;
        planes[LEFT].z = matrix[2].w + matrix[2].x;
        planes[LEFT].w = matrix[3].w + matrix[3].x;

        planes[RIGHT].x = matrix[0].w - matrix[0].x;
        planes[RIGHT].y = matrix[1].w - matrix[1].x;
        planes[RIGHT].z = matrix[2].w - matrix[2].x;
        planes[RIGHT].w = matrix[3].w - matrix[3].x;

        planes[TOP].x = matrix[0].w - matrix[0].y;
        planes[TOP].y = matrix[1].w - matrix[1].y;
        planes[TOP].z = matrix[2].w - matrix[2].y;
        planes[TOP].w = matrix[3].w - matrix[3].y;

        planes[BOTTOM].x = matrix[0].w + matrix[0].y;
        planes[BOTTOM].y = matrix[1].w + matrix[1].y;
        planes[BOTTOM].z = matrix[2].w + matrix[2].y;
        planes[BOTTOM].w = matrix[3].w + matrix[3].y;

        planes[BACK].x = matrix[0].w + matrix[0].z;
        planes[BACK].y = matrix[1].w + matrix[1].z;
        planes[BACK].z = matrix[2].w + matrix[2].z;
        planes[BACK].w = matrix[3].w + matrix[3].z;

        planes[FRONT].x = matrix[0].w - matrix[0].z;
        planes[FRONT].y = matrix[1].w - matrix[1].z;
        planes[FRONT].z = matrix[2].w - matrix[2].z;
        planes[FRONT].w = matrix[3].w - matrix[3].z;

        for (size_t i = 0; i < planes.size(); i++)
        {
            float length = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
            planes[i] /= length;
        }


        // 裁剪空间的 8 个点
        static glm::vec4 frustumCornersClipSpace[8] = {
            {-1.0f,  1.0f, -1.0f, 1.0f}, // 近裁剪面左上
            { 1.0f,  1.0f, -1.0f, 1.0f}, // 近裁剪面右上
            {-1.0f, -1.0f, -1.0f, 1.0f}, // 近裁剪面左下
            { 1.0f, -1.0f, -1.0f, 1.0f}, // 近裁剪面右下
            {-1.0f,  1.0f,  1.0f, 1.0f}, // 远裁剪面左上
            { 1.0f,  1.0f,  1.0f, 1.0f}, // 远裁剪面右上
            {-1.0f, -1.0f,  1.0f, 1.0f}, // 远裁剪面左下
            { 1.0f, -1.0f,  1.0f, 1.0f}, // 远裁剪面右下
        };




    }

    bool Frustum::check_sphere(glm::vec3 pos, float radius)
    {
        for (size_t i = 0; i < planes.size(); i++)
        {
            if ((planes[i].x * pos.x) + (planes[i].y * pos.y) + (planes[i].z * pos.z) + planes[i].w <= -radius)
            {
                return false;
            }
        }
        return true;
    }
    const std::array<glm::vec4, 6> &Frustum::get_planes() const
    {
        return planes;
    }

    const std::array<glm::vec4, 6> & Frustum::get_corners() const
    {

    }
}   // namespace vkb