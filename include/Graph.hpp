//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// UGen graph values. A UGen primitive returns a Sig, a reference to one output of a
// UGenNode, in place of a sample stream. >sd compiles the graph reachable from its
// channels to SCgf, SuperCollider's SynthDef file format.

#ifndef SAPF_GRAPH_HPP
#define SAPF_GRAPH_HPP

#include "VM.hpp"
#include <string>
#include <vector>

enum { rateIR = 0, rateKR = 1, rateAR = 2, rateDR = 3 };

// SC operator special indices (BinaryOpUGens.cpp, UnaryOpUGens.cpp).
enum {
	opAdd = 0, opSub = 1, opMul = 2, opIDiv = 3, opFDiv = 4, opMod = 5,
	opEQ = 6, opNE = 7, opLT = 8, opGT = 9, opLE = 10, opGE = 11,
	opMin = 12, opMax = 13, opLCM = 17, opGCD = 18, opRound = 19, opRoundUp = 20, opTrunc = 21,
	opAtan2 = 22, opHypot = 23, opPow = 25,
	opRing1 = 30, opRing2 = 31, opRing3 = 32, opRing4 = 33,
	opDifSqr = 34, opSumSqr = 35, opSqrSum = 36, opSqrDif = 37, opAbsDif = 38,
	opThresh = 39, opAMClip = 40, opScaleNeg = 41, opClip2 = 42, opExcess = 43, opFold2 = 44, opWrap2 = 45
};
enum {
	opNeg = 0, opAbs = 5, opCeil = 8, opFloor = 9, opFrac = 10, opSign = 11, opSquared = 12, opCubed = 13,
	opSqrt = 14, opExp = 15, opRecip = 16, opMIDICPS = 17, opCPSMIDI = 18, opMIDIRatio = 19, opRatioMIDI = 20,
	opDbAmp = 21, opAmpDb = 22, opOctCPS = 23, opCPSOct = 24, opLog = 25, opLog2 = 26, opLog10 = 27,
	opSin = 28, opCos = 29, opTan = 30, opArcSin = 31, opArcCos = 32, opArcTan = 33,
	opSinH = 34, opCosH = 35, opTanH = 36, opDistort = 42, opSoftClip = 43
};

class UGenNode : public Object
{
public:
	std::string name;
	int rate;
	int special;
	int numOutputs;
	bool pure;
	std::vector<V> inputs;   // each is a real (constant) or a Sig
	int64_t serial;          // creation order; the topological sort breaks ties by it
	// A synth parameter. Compilation replaces it with an output of a Control UGen.
	bool isParam = false;
	double paramDefault = 0.;
	// Compilation lowers an audio-rate node to control rate when all its consumers are control
	// rate, unless its rate is fixed: soundin and ugen nodes, whose meaning depends on the rate.
	bool fixedRate = false;
	P<UGenNode> krOf;        // set on the node kr returns: the audio-rate node it copies
	bool endGate = false;    // an EnvGen from endAfter: 1, then 0 when a finite signal ends
	P<Object> texture;       // set on the In node ola returns: the voices that write its bus
	P<Object> wavetable;     // set on osc's VOsc: the tables it reads, from buffer tableBuf
	int tableBuf = -1;
	P<Object> sample;        // set on sf>'s and playbuf's PlayBuf: the sound file it reads into tableBuf

	UGenNode(const char* inName, int inRate, std::vector<V> inInputs, int inNumOutputs, int inSpecial, bool inPure);

	virtual const char* TypeName() const override { return "UGenNode"; }
};

class Sig : public Object
{
public:
	P<UGenNode> node;
	int index;

	Sig(P<UGenNode> const& inNode, int inIndex) : node(inNode), index(inIndex) { finite = true; }

	int rate() const { return node->rate; }

	virtual const char* TypeName() const override { return "Sig"; }
	virtual void print(Thread& th, std::string& out, int depth) override;

	virtual V unaryOp(Thread& th, UnaryOp* op) override;
	virtual V binaryOp(Thread& th, BinaryOp* op, Arg _b) override;
	virtual V binaryOpWithReal(Thread& th, BinaryOp* op, Z _a) override;
	virtual V binaryOpWithVList(Thread& th, BinaryOp* op, List* _a) override;
	virtual V binaryOpWithZList(Thread& th, BinaryOp* op, List* _a) override;
};

inline bool isSig(Arg v) { return v.o() && dynamic_cast<Sig*>(v.o()) != nullptr; }
inline int rateOf(Arg v) { return isSig(v) ? ((Sig*)v.o())->rate() : rateIR; }

// Pops a UGen input: a real or a Sig.
V popGraphIn(Thread& th, const char* msg);

// Makes a UGen. Returns a Sig for one output, a list of Sigs for several.
V makeUGen(const char* name, int rate, std::vector<V> inputs, int numOutputs = 1, int special = 0, bool pure = true,
	bool fixedRate = false);

// sapf's kr: a control-rate version of `v`. Reals and control-rate Sigs return unchanged.
V toKR(Arg v);

// SC operators by special index. Folds when the inputs are constants.
V graphUnary(Thread& th, int special, Arg a);
V graphBinary(Thread& th, int special, Arg a, Arg b);

// `in` for `frames` frames, then zero; the synth frees itself. sapf's N and T on a UGen.
V endAfter(Thread& th, Arg in, Arg frames);
V secondsToFrames(Thread& th, double seconds);

// Synth parameter named `name` with default `value`.
V makeParam(const char* name, double value);

// Compiles channels (a Sig, a real, or a finite list of them) to an SCgf file holding one def that
// writes them from bus `outBus`.
std::string compileSynthDef(Thread& th, Arg channels, const std::string& name, int outBus = 0);

// Seconds until the channels end at sample rate `sr`, as sapf's >sf ends a file: when its first
// finite channel ends. A channel ends at its first end (endAfter, or an EnvGen that frees the synth).
// Throws, with a message, when no channel ends or an end depends on a signal.
double renderSeconds(Thread& th, Arg channels, double sr);

// How many channels compileSynthDef writes for `channels`.
int channelCount(Thread& th, Arg channels);

// The textures (ola voices) the channels read.
std::vector<P<Object>> texturesOf(Thread& th, Arg channels);

// The VOsc nodes of osc that the channels read.
std::vector<UGenNode*> wavetableNodesOf(Thread& th, Arg channels);

// The PlayBuf nodes of sf> and playbuf that the channels read.
std::vector<UGenNode*> sampleNodesOf(Thread& th, Arg channels);

#endif
