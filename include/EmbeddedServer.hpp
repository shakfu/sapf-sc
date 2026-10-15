//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// scsynth in this process, for the sapf executable; see cmake/EmbeddedScsynth.cmake.

#ifndef SAPF_EMBEDDEDSERVER_HPP
#define SAPF_EMBEDDEDSERVER_HPP

// Makes play and stop use an scsynth this process starts on the first play, unless $STAC_SCSYNTH
// names a server to reach over UDP. Settings, read when it starts:
//   $STAC_DEVICE    the audio device, by name; default the system's
//   $STAC_OUTPUTS   output channels, default 2
//   $STAC_INPUTS    input channels, default 0
//   $STAC_SR        the sample rate, default the device's
//   $STAC_PLUGINS   the UGen plugin folder, default plugins/ beside the executable
void registerEmbeddedScsynth();

// Returns once the embedded scsynth, if it started, runs no synths. Ctrl-C ends the wait.
void waitForEmbeddedScsynth();

#endif
