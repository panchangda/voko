#pragma once
#include <vector>
#include <RenderPass.h>

class SceneRenderer
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;
    
    virtual void Render();
};
