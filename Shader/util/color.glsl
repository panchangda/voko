/**
    .vh: voko header
    Coloring Helpers
*
*/

#ifndef COLOR_VH
#define COLOR_VH

vec3 colorCorrection(vec3 color, float gamma){
    return pow(color, vec3(gamma));
}
vec3 gammaToLinear(vec3 gammaColor){
    return pow(gammaColor, vec3(2.2));
}
vec3 linearToGamma(vec3 linearColor){
    return pow(linearColor, vec3(1/2.2));
}

// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 ReinhardTonemap(vec3 color){
    return color / (color + vec3(1.0));
}

#endif // COLOR_VH



