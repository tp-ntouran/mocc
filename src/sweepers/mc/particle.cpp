/*
   Copyright 2016 Mitchell Young

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <iostream>

#include "particle.hpp"

namespace mocc {
    std::ostream &operator<<(std::ostream &os, const Particle &p) {
        os << "Global position: " << p.location_global.x << " "
                                  << p.location_global.y << " "
                                  << p.location_global.z << std::endl;
        os << "Pin-local position: " << p.location.x << " "
                                     << p.location.y << std::endl;
        os << "Direction: " << p.direction << std::endl;
        return os;
    }
}