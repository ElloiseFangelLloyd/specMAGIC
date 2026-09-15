#pragma once

#include <stdexcept>
#include <string>
#include <array>
#include <cmath>
#include "constants.hpp"
#include "types.hpp"
#include "satellite_info.hpp"
#include "navigation.hpp"

namespace Satellite {

    MAGIC_EXACT calcObsTime(unsigned int line, Metadata info, DateTime time);

    int flipVertical(int line, int height);

    void geo2Image(MAGIC_EXACT lat_rad, MAGIC_EXACT lon_rad, Metadata& metadata,
        unsigned int& col, unsigned int& line);


}