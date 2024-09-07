#pragma once

#include <string>
#include "Light.h"


class PointLight : public Light
{
public:
    explicit PointLight(const std::string &name):
    Light(name) {
        set_light_type(LightType::Point);
    }
};
