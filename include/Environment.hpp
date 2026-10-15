//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#ifndef SAPF_ENVIRONMENT_HPP
#define SAPF_ENVIRONMENT_HPP

#include <cstdlib>
#include <string>

// $STAC_<name>.
inline const char* stacEnv(const char* name) { return getenv(("STAC_" + std::string(name)).c_str()); }

#endif
