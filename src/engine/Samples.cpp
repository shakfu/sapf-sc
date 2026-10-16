//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "Samples.hpp"
#include "Namespace.hpp"
#include "Resources.hpp"
#include "Scsynth.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

namespace {

// Buffers for sound files: the 64 scsy's sidecar can name.
const int kSampleBufs = 64;

// A sound file a PlayBuf reads.
class SampleFile : public Object
{
public:
	std::string path;      // as written
	std::string absolute;
	int channels;
	int64_t frames;

	SampleFile(std::string p, std::string a, int c, int64_t f) : path(std::move(p)), absolute(std::move(a)), channels(c), frames(f) {}

	virtual const char* TypeName() const override { return "SampleFile"; }
};

std::mutex gMutex;
std::vector<std::string> gBufs;  // the absolute path in each buffer, from 0

int bufferFor(const std::string& absolute)
{
	std::lock_guard<std::mutex> lock(gMutex);
	for (size_t i = 0; i < gBufs.size(); ++i)
		if (gBufs[i] == absolute) return (int)i;
	for (size_t i = 0; i < gBufs.size(); ++i)
		if (gBufs[i].empty()) {
			gBufs[i] = absolute;
			return (int)i;
		}
	if ((int)gBufs.size() == kSampleBufs) {
		post("sf> : more than %d sound files\n", kSampleBufs);
		throw errOutOfRange;
	}
	gBufs.push_back(absolute);
	return (int)gBufs.size() - 1;
}

uint32_t le32(const char* p) { return (uint8_t)p[0] | (uint8_t)p[1] << 8 | (uint8_t)p[2] << 16 | (uint32_t)(uint8_t)p[3] << 24; }
uint16_t le16(const char* p) { return (uint16_t)((uint8_t)p[0] | (uint8_t)p[1] << 8); }
uint32_t be32(const char* p) { return (uint32_t)(uint8_t)p[0] << 24 | (uint8_t)p[1] << 16 | (uint8_t)p[2] << 8 | (uint8_t)p[3]; }
uint16_t be16(const char* p) { return (uint16_t)((uint8_t)p[0] << 8 | (uint8_t)p[1]); }

// Channels and frames from a WAV or AIFF header. PlayBuf needs the channel count when the def
// compiles; scsynth reads the samples.
bool readHeader(const std::string& path, int& channels, int64_t& frames)
{
	std::ifstream f(path, std::ios::binary);
	char head[12];
	if (!f.read(head, 12)) return false;
	bool wav = !memcmp(head, "RIFF", 4) && !memcmp(head + 8, "WAVE", 4);
	bool aiff = !memcmp(head, "FORM", 4) && (!memcmp(head + 8, "AIFF", 4) || !memcmp(head + 8, "AIFC", 4));
	if (!wav && !aiff) return false;
	int blockAlign = 0;
	channels = 0;
	frames = -1;
	char chunk[8];
	while (f.read(chunk, 8)) {
		uint32_t size = wav ? le32(chunk + 4) : be32(chunk + 4);
		std::streampos next = f.tellg() + (std::streamoff)(size + (size & 1));
		char body[18] = {};
		if (wav && !memcmp(chunk, "fmt ", 4) && f.read(body, 16)) {
			channels = le16(body + 2);
			blockAlign = le16(body + 12);
		} else if (wav && !memcmp(chunk, "data", 4)) {
			if (blockAlign > 0) frames = size / blockAlign;
		} else if (aiff && !memcmp(chunk, "COMM", 4) && f.read(body, 6)) {
			channels = be16(body);
			frames = be32(body + 2);
		}
		f.seekg(next);
	}
	return channels > 0 && frames >= 0;
}

P<SampleFile> openSample(Thread& th, Arg name, const char* msg)
{
	if (!name.isString()) wrongType(msg, "String", name);
	std::string path = ((String*)name.o())->s;
	std::error_code ec;
	std::string absolute = std::filesystem::absolute(path, ec).lexically_normal().string();
	int channels;
	int64_t frames;
	if (!readHeader(absolute, channels, frames)) {
		post("%s : cannot read '%s' as a WAV or AIFF file\n", msg, path.c_str());
		throw errFailed;
	}
	return new SampleFile(path, absolute, channels, frames);
}

// PlayBuf's channels from buffer `buf`, as a list as sapf's sf> gives.
V playBuf(Thread& th, P<SampleFile> const& file, Arg rate, double loop, double doneAction)
{
	int buf = bufferFor(file->absolute);
	V out = makeUGen("PlayBuf", rateAR, {(double)buf, rate, 1., 0., loop, doneAction}, file->channels, 0, false);
	UGenNode* node = isSig(out) ? ((Sig*)out.o())->node() : ((Sig*)((List*)out.o())->at(0).o())->node();
	node->sample = file;
	node->tableBuf = buf;
	if (isSig(out)) {
		P<List> list = new List(itemTypeV, 1);
		list->add(out);
		out = list;
	}
	return out;
}

V endEach(Thread& th, Arg channels, Arg frames)
{
	P<List> in = (List*)channels.o();
	P<List> out = new List(itemTypeV, in->mArray->size());
	for (int64_t i = 0; i < in->mArray->size(); ++i) out->add(endAfter(th, in->mArray->at(i), frames));
	return out;
}

} // namespace

void releaseSampleBuffer(int buf)
{
	std::lock_guard<std::mutex> lock(gMutex);
	if (buf >= 0 && buf < (int)gBufs.size()) gBufs[buf].clear();
}

SampleInfo sampleBuffer(Thread& th, Arg path, const char* msg)
{
	P<SampleFile> file = openSample(th, path, msg);
	return {bufferFor(file->absolute), file->channels, file->frames, file->absolute};
}

std::vector<std::string> loadSamples(Thread& th, Arg channels, LoadedTables& loaded, bool absolute)
{
	std::vector<std::string> out;
	std::lock_guard<std::mutex> lock(loaded.mutex);
	for (UGenNode* u : sampleNodesOf(th, channels)) {
		if (!loaded.bufs.insert(u->tableBuf).second) continue;
		SampleFile* f = (SampleFile*)u->sample();
		out.push_back(oscMessage("/b_allocRead", {{OscArg::Int, u->tableBuf, ""},
			{OscArg::String, 0, absolute ? f->absolute : f->path}}));
	}
	return out;
}

std::string sidecarWithSamples(Thread& th, Arg channels, const std::string& sidecar)
{
	std::istringstream lines(sidecar);
	std::string line, out;
	while (std::getline(lines, line))
		if (line.compare(0, 2, "//") != 0 || line.find("@buf") == std::string::npos) out += line + "\n";
	std::vector<int> seen;
	for (UGenNode* u : sampleNodesOf(th, channels)) {
		if (std::find(seen.begin(), seen.end(), u->tableBuf) != seen.end()) continue;
		seen.push_back(u->tableBuf);
		out += "// @buf " + std::to_string(u->tableBuf) + " " + ((SampleFile*)u->sample())->path + "\n";
	}
	return out;
}

////////////////////////////////////////////////////////////////////////////////

// sapf's sf> reads the file's frames at its own rate, so PlayBuf plays at rate 1, not BufRateScale.
static void sfread_(Thread& th, Prim* prim)
{
	P<SampleFile> file = openSample(th, th.pop(), "sf>");
	th.push(endEach(th, playBuf(th, file, 1., 0., 0.), (double)file->frames));
}

// A real rate ends the signal after the file; a UGen rate lets PlayBuf free the synth at the end.
static void playbuf_(Thread& th, Prim* prim)
{
	double loop = th.popFloat("playbuf : loop");
	V rate = popGraphIn(th, "playbuf : rate");
	P<SampleFile> file = openSample(th, th.pop(), "playbuf");
	if (loop != 0.) {
		th.push(playBuf(th, file, rate, 1., 0.));
	} else if (rate.isReal()) {
		if (rate.f <= 0.) {
			post("playbuf : rate must be positive when not looping\n");
			throw errOutOfRange;
		}
		th.push(endEach(th, playBuf(th, file, rate, 0., 0.), ceil(file->frames / rate.f)));
	} else {
		th.push(playBuf(th, file, rate, 0., 2.));
	}
}

void AddSampleOps()
{
	SapfWords sapf;
	vm.def("sf>", 1, 1, sfread_, "(filename --> channels) a WAV or AIFF file's channels, played once at rate 1 from an scsynth buffer. PlayBuf.");
	vm.def("playbuf", 3, 1, playbuf_, "(filename rate loop --> channels) a WAV or AIFF file's channels at rate, looped when loop is not 0. PlayBuf.");
}
