#version 450

layout (location = 0) out vec2 outUV;

/*
    ref to https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
    vulkan is right handed, meaning its Y is now flipped,
    left top is 0,0; right down is 1,1
*/

void main()
{
    outUV = vec2(0.0);
    // only 3 vertices
    switch ( gl_VertexIndex ){

            case(0):
                outUV = vec2(0.0, 0.0);
                break;
            case(1):
                outUV = vec2(0.0, 2.0);
                break;
            case(2):
                outUV = vec2(2.0, 0.0);
                break;

    }
    // original bit-wise compute, unacceptable for readability
    // outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}