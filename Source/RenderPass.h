

enum RenderPassType
{
    Mesh = 0x01,
    FullScreen = 0x02,
};
class RenderPass
{
    RenderPassType PassType = Mesh;
    RenderPass();

    virtual ~RenderPass();
};