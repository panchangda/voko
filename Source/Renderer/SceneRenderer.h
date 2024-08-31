#pragma once
#include <vector>
#include <RenderPass/RenderPass.h>

class SceneRenderer
{
public:
    SceneRenderer() = default;
    virtual ~SceneRenderer() = default;
    
    virtual void Render();
};
