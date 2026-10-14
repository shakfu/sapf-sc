//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// Sound files: sf> and playbuf play them with PlayBuf from scsynth buffers 0..63, which play and
// >nrt load and >sd names in its sidecar.

#ifndef SAPF_SAMPLES_HPP
#define SAPF_SAMPLES_HPP

#include "Wavetables.hpp"
#include <string>
#include <vector>

// /b_allocRead for each sound file the channels read that `loaded` lacks. With `absolute`, relative
// paths become absolute, for a server on this machine; otherwise they stay as written.
std::vector<std::string> loadSamples(Thread& th, Arg channels, LoadedTables& loaded, bool absolute);

// `sidecar`, the text of a .txt beside a def, with its `// @buf` lines replaced by one per sound
// file the channels read. Its other lines stay.
std::string sidecarWithSamples(Thread& th, Arg channels, const std::string& sidecar);

void AddSampleOps();

#endif
