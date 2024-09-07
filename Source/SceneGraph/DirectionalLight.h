#pragma once

#include <string>
#include "Light.h"


class DirectionalLight : public Light
{
public:
    explicit DirectionalLight(const std::string &name):
    Light(name) {
        set_light_type(LightType::Directional);
    }
};
