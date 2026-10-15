//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "Wavetables.hpp"
#include "Namespace.hpp"
#include "Scsynth.hpp"
#include "MathFuns.hpp"
#include <algorithm>
#include <cmath>

namespace {

// sapf's layout: table i holds kHarmonics[i] harmonics. sapf's tables have 16384 samples; 4096
// holds the 1024 harmonics of the last table.
const int kNumTables = 30, kTableSize = 4096, kMaxHarmonics = 1290;
const int kHarmonics[kNumTables] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 25, 32, 40, 50, 64, 80, 101, 128,
	161, 203, 256, 322, 406, 512, 645, 812, 1024};

// Buffers for wave tables, above the ones a user is likely to allocate; scsynth has 1024 by default.
const int kFirstBuf = 512, kLastBuf = 1024;

std::mutex gBufMutex;
// Each wave table keeps its buffers; holding the List keeps its address from being reused.
std::vector<std::pair<P<List>, int>> gBufs;
int gNextBuf = kFirstBuf;

int bufferFor(P<List> const& table)
{
	std::lock_guard<std::mutex> lock(gBufMutex);
	for (auto& b : gBufs)
		if (b.first() == table()) return b.second;
	if (gNextBuf + kNumTables > kLastBuf) {
		gNextBuf = kFirstBuf;
		gBufs.clear();
	}
	int buf = gNextBuf;
	gNextBuf += kNumTables;
	gBufs.push_back({table, buf});
	return buf;
}

// sapf's fillWaveTable as its code describes it: harmonic k + 1 is
// amps[k] * cos(k * pi / 2 / n)^smooth * sin((k + 1) * theta + 2 pi phases[k]). All 30 tables
// are normalized together. Original sapf's own tables are broken (docs/dev/upstream-sapf-bugs.md).
P<List> makeTables(int n, const Z* amps, int ampStride, const Z* phases, int phaseStride, double smooth)
{
	P<List> list = new List(itemTypeZ, kNumTables * kTableSize);
	Z* out = list->mArray->z();
	std::fill(out, out + kNumTables * kTableSize, 0.);
	for (int t = 0; t < kNumTables; ++t) {
		int nh = std::min(n, kHarmonics[t]);
		double w = M_PI_2 / nh;
		Z* table = out + t * kTableSize;
		for (int k = 0; k < nh; ++k) {
			double amp = amps[k * ampStride] * (smooth == 0. ? 1. : pow(cos(w * k), smooth));
			if (amp == 0.) continue;
			// sin and cos of (k + 1) * theta + phase by rotation.
			double step = 2. * M_PI * (k + 1) / kTableSize, c = cos(step), s = sin(step);
			double phase = 2. * M_PI * phases[k * phaseStride];
			double x = sin(phase), y = cos(phase);
			for (int j = 0; j < kTableSize; ++j) {
				table[j] += amp * x;
				double x1 = x * c + y * s;
				y = y * c - x * s;
				x = x1;
			}
		}
	}
	Z peak = 0.;
	for (int i = 0; i < kNumTables * kTableSize; ++i) peak = std::max(peak, fabs(out[i]));
	if (peak > 0.)
		for (int i = 0; i < kNumTables * kTableSize; ++i) out[i] /= peak;
	list->mArray->setSize(kNumTables * kTableSize);
	return list;
}

bool isWavetable(Arg v)
{
	return v.isZList() && ((List*)v.o())->isPacked() && ((List*)v.o())->mArray->size() == kNumTables * kTableSize;
}

// One /b_alloc, completed by /b_setn with the table in SC's wavetable format: pairs of
// 2a - b and b - a for consecutive samples a and b.
std::string loadTable(int buf, const Z* table)
{
	std::vector<OscArg> setn = {{OscArg::Int, buf, ""}, {OscArg::Int, 0, ""}, {OscArg::Int, 2 * kTableSize, ""}};
	for (int i = 0; i < kTableSize; ++i) {
		double a = table[i], b = table[(i + 1) % kTableSize];
		setn.push_back({OscArg::Float, 0, "", (float)(2. * a - b)});
		setn.push_back({OscArg::Float, 0, "", (float)(b - a)});
	}
	return oscMessage("/b_alloc", {{OscArg::Int, buf, ""}, {OscArg::Int, 2 * kTableSize, ""}, {OscArg::Int, 1, ""},
		{OscArg::Blob, 0, oscMessage("/b_setn", setn)}});
}

} // namespace

std::vector<std::string> loadWavetables(Thread& th, Arg channels, LoadedTables& loaded)
{
	std::vector<std::string> out;
	std::lock_guard<std::mutex> lock(loaded.mutex);
	for (UGenNode* u : wavetableNodesOf(th, channels)) {
		if (!loaded.bufs.insert(u->tableBuf).second) continue;
		const Z* z = ((List*)u->wavetable())->mArray->z();
		for (int t = 0; t < kNumTables; ++t) out.push_back(loadTable(u->tableBuf + t, z + t * kTableSize));
	}
	return out;
}

////////////////////////////////////////////////////////////////////////////////

static void wavefill_(Thread& th, Prim* prim)
{
	double smooth = th.popFloat("wavefill : smooth");
	V phases = th.pop();
	V amps = th.pop();
	int64_t n = kMaxHarmonics;
	P<List> phasel, ampl;
	const Z* phasez = &phases.f;
	const Z* ampz = &amps.f;
	int phaseStride = 0, ampStride = 0;
	if (phases.isZList()) {
		phasel = ((List*)phases.o())->packSome(th, n);
		phasez = phasel->mArray->z();
		phaseStride = 1;
	} else if (!phases.isReal()) {
		wrongType("wavefill : phases", "Real or signal", phases);
	}
	if (amps.isZList()) {
		ampl = ((List*)amps.o())->packSome(th, n);
		ampz = ampl->mArray->z();
		ampStride = 1;
	} else if (!amps.isReal()) {
		wrongType("wavefill : amps", "Real or signal", amps);
	}
	th.push(makeTables((int)n, ampz, ampStride, phasez, phaseStride, smooth));
}

// osc picks tables as sapf does: h = freqLimit / |freq| harmonics fit below the limit, and the
// fractional table is h - 1 up to 8 harmonics and 3 log2(h) - 2 above. VOsc crossfades between
// neighbouring tables linearly; sapf uses an S curve.
static void osc_(Thread& th, Prim* prim)
{
	V table = th.pop();
	double phase = th.popFloat("osc : phase");
	V freq = popGraphIn(th, "osc : freq");
	if (!isWavetable(table)) wrongType("osc : wavetable", "wave table from wavefill or sawTbl", table);
	int buf = bufferFor((List*)table.o());

	V sr = makeUGen("SampleRate", rateIR, {});
	V limit = graphBinary(th, opMin, graphBinary(th, opMul, sr, .5), 24000.);
	V afreq = graphBinary(th, opMax, graphUnary(th, opAbs, freq), 1e-9);
	V h = graphBinary(th, opMin, graphBinary(th, opFDiv, limit, afreq), (double)kMaxHarmonics);
	V low = graphBinary(th, opSub, h, 1.);
	V high = graphBinary(th, opSub, graphBinary(th, opMul, graphUnary(th, opLog2, h), 3.), 2.);
	V above = graphBinary(th, opGE, h, 8.);
	V ft = graphBinary(th, opAdd, low, graphBinary(th, opMul, graphBinary(th, opSub, high, low), above));
	ft = graphBinary(th, opMax, graphBinary(th, opMin, ft, kNumTables - 1.0000001), 0.);
	V bufpos = graphBinary(th, opAdd, ft, (double)buf);

	V out = makeUGen("VOsc", rateAR, {bufpos, freq, 2. * M_PI * sc_wrap(phase, 0., 1.)});
	UGenNode* node = ((Sig*)out.o())->node();
	node->wavetable = table.o();
	node->tableBuf = buf;
	th.push(out);
}

// sapf's classic tables, built on first use.
static P<List> classic(int which)
{
	static P<List> tables[4];
	if (tables[which]()) return tables[which];
	std::vector<Z> amps(kMaxHarmonics + 1), phases(kMaxHarmonics + 1, 0.);
	for (int i = 1; i <= kMaxHarmonics; ++i) {
		switch (which) {
			case 0 : amps[i] = 1. / (i * i); break;                                              // parabolic
			case 1 : amps[i] = i % 2 ? (i % 4 == 1 ? 1. : -1.) / (i * i) : 0.; break;            // triangle
			case 2 : amps[i] = i % 2 ? 1. / i : 0.; break;                                       // square
			case 3 : amps[i] = 1. / i; phases[i] = i % 2 ? 0. : .5; break;                      // sawtooth
		}
	}
	double parPhase = .25;
	tables[which] = which == 0 ? makeTables(kMaxHarmonics, &amps[1], 1, &parPhase, 0, 0.)
	              : makeTables(kMaxHarmonics, &amps[1], 1, &phases[1], which == 3 ? 1 : 0, 0.);
	return tables[which];
}

static void parTbl_(Thread& th, Prim* prim) { th.push(classic(0)); }
static void triTbl_(Thread& th, Prim* prim) { th.push(classic(1)); }
static void sqrTbl_(Thread& th, Prim* prim) { th.push(classic(2)); }
static void sawTbl_(Thread& th, Prim* prim) { th.push(classic(3)); }

void AddWavetableOps()
{
	vm.addBifHelp("\n*** wave tables ***");
	SapfWords sapf;
	vm.defautomap("wavefill", "aak", wavefill_, "(amps phases smooth --> wavetable) generates a set of 1/3 octave wavetables for table lookup oscillators. sin(i*theta + 2pi*phases[i])*amps[i]*pow(cos(pi/2*i/n), smooth). smoothing reduces Gibb's phenomenon. zero is no smoothing");
	vm.defmcx("osc", 3, osc_, "(freq phase wavetable --> out) band limited wave table oscillator. wavetable is a table created with wavefill. VOsc over scsynth buffers that play and >nrt load.");
	vm.def("parTbl", 0, 1, parTbl_, "(--> wavetable) parabolic wave table.");
	vm.def("triTbl", 0, 1, triTbl_, "(--> wavetable) triangle wave table.");
	vm.def("sqrTbl", 0, 1, sqrTbl_, "(--> wavetable) square wave table.");
	vm.def("sawTbl", 0, 1, sawTbl_, "(--> wavetable) sawtooth wave table.");
}
