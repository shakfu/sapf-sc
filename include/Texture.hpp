//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// sapf's ola (overlap add) as SuperCollider synths: each sound is its own synth writing to a
// private bus, and ola returns In.ar on that bus. play and >nrt start the voices on schedule.

#ifndef SAPF_TEXTURE_HPP
#define SAPF_TEXTURE_HPP

#include "Graph.hpp"
#include "Wavetables.hpp"
#include <memory>
#include <string>
#include <vector>

// Starts a scheduler thread for each texture the channels read; voice times count from
// `startTime`, a Unix time. Voices load buffers that `loaded` lacks. A scheduler stops when synth
// `mainNode`, which reads its voices, has ended.
void playTextures(Thread& th, Arg channels, double startTime, std::shared_ptr<LoadedTables> loaded, int mainNode);

// Ends the scheduler threads.
void stopTextures();

// The scheduler threads still running. They are detached; a host that unloads the interpreter's code
// waits for none to run after stopTextures, and a moment more: a thread's arguments are destroyed
// after it counts itself out.
int runningTextures();

// The NRT command file for channels with their voices up to `seconds`, the main def named `name`.
std::string nrtScore(Thread& th, Arg channels, const std::string& name, double seconds);

void AddTextureOps();

#endif
