//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// Event streams (PatternOps.cpp): play and stop hand them on here.

#ifndef SAPF_PATTERNS_HPP
#define SAPF_PATTERNS_HPP

#include "VM.hpp"

// Whether v is a list whose first item is a Form: events, not channels.
bool isEventStream(Thread& th, Arg v);

// Schedules the events from now on a thread of their own and pushes a Player; .stop ends it.
void playEventStream(Thread& th, Arg stream);

// Ends every player and forgets every ndef's synth, as stop does.
void stopEventStreams();

#endif
