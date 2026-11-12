#pragma once
#include "Geometry.hpp"

// Native wire interpretation of the supplied PulseGrid effect: a rounded
// lattice cube, nested pulse shells, orbital rails, floor grid and beat rings.
// Time is quantized to the source effect's 25 Hz visual cadence.
namespace geo {
Mesh3 pulseGrid(float seconds = 0.f);
}
