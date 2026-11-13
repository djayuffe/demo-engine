#pragma once
#include "Geometry.hpp"

// Standalone PulseGrid Reactor model: rounded chassis, articulated pulse core,
// six coiled power conduits, orbital gimbals, floor grid and beat rings. Time
// is quantized to the source effect's 25 Hz visual cadence.
namespace geo {
Mesh3 pulseGrid(float seconds = 0.f);
// PAL-era reinterpretation: block geometry, voxel core, eight-sided coils and
// extruded C64 pixel lettering, animated at the C64's 50 Hz PAL cadence.
Mesh3 c64PulseGrid(float seconds = 0.f);
}
