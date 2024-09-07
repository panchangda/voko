#pragma once

#include <string>
#include "Light.h"


class SpotLight : public Light
{
public:
   explicit SpotLight(const std::string& name):
   Light(name) {
      set_light_type(LightType::Spot);
   }
};
