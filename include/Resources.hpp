//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// Server objects with an index: Bus, Buffer and Node (docs/dev/stac.md, Server objects). As a UGen
// input or a control value, each is its index.

#ifndef SAPF_RESOURCES_HPP
#define SAPF_RESOURCES_HPP

#include "VM.hpp"

class ServerResource : public Object
{
public:
	int mIndex;

	explicit ServerResource(int index) : mIndex(index) {}
	virtual Z asFloat() const override { return mIndex; }
};

inline ServerResource* asResource(Arg v) { return v.o() ? dynamic_cast<ServerResource*>(v.o()) : nullptr; }

// The lowest private audio bus bus.audio has taken; ola's buses stay below it (Texture.cpp).
int audioBusFloor();

// The sound-file buffer for `path`, as sf> and playbuf use, and the file's channels and frames
// (Samples.cpp). Throws when the file is not a WAV or AIFF file, or 64 files are in use.
struct SampleInfo {
	int buf;
	int channels;
	int64_t frames;
	std::string absolute;
};
SampleInfo sampleBuffer(Thread& th, Arg path, const char* msg);

#endif
