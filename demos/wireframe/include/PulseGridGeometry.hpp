#pragma once
#include "Geometry.hpp"

// Standalone PulseGrid Reactor model: rounded chassis, articulated pulse core,
// six coiled power conduits, orbital gimbals, floor grid and beat rings. Time
// is quantized to the source effect's 25 Hz visual cadence.
namespace geo {
Mesh3 pulseGrid(float seconds = 0.f);
}
