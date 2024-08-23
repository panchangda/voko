#pragma once

#include <string>

enum RenderPassType
{
    Mesh = 0x01,
    FullScreen = 0x02,
};
class RenderPass
{
public:
    RenderPass();
    virtual ~RenderPass();
    
    RenderPass(const std::string& name):passName(name){}
    std::string get_name(){return passName;}
    
    RenderPassType PassType = Mesh;
    
    virtual void setupFrameBuffer();
    virtual void setupDescriptors();
    virtual void buildCommandBuffer();

private:
    std::string passName;
};