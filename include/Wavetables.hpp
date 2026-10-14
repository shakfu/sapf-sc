//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// sapf's wave tables (wavefill, sawTbl and the others) and osc. A wave table is 30 third-octave
// tables; osc plays them with VOsc over 30 consecutive scsynth buffers that play and >nrt load.

#ifndef SAPF_WAVETABLES_HPP
#define SAPF_WAVETABLES_HPP

#include "Graph.hpp"
#include <mutex>
#include <set>
#include <string>
#include <vector>

// The buffers one play or >nrt score has loaded; ola's scheduler threads share it.
struct LoadedTables {
	std::mutex mutex;
	std::set<int> bufs;
};

// Messages that allocate and fill the buffers of the wave tables the channels read and `loaded`
// lacks: one /b_alloc per table, with /b_setn as its completion message.
std::vector<std::string> loadWavetables(Thread& th, Arg channels, LoadedTables& loaded);

void AddWavetableOps();

#endif
