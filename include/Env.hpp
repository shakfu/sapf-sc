//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// stac's Env: SC's Env, the breakpoints EnvGen and IEnvGen read (docs/dev/stac.md, Envelopes).
// Levels, times and curves may be reals, UGens or lists, which expand to channels in the UGen.

#ifndef SAPF_ENV_HPP
#define SAPF_ENV_HPP

#include "VM.hpp"
#include <vector>

class Env : public Object
{
public:
	std::vector<V> levels;
	std::vector<V> times;   // one per segment
	std::vector<V> curves;  // read with wrap per segment: a real or UGen curvature, or a shape symbol
	V releaseNode = -99.;   // SC's nil
	V loopNode = -99.;
	V offset = 0.;

	virtual const char* TypeName() const override { return "Env"; }
	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override;

	// EnvGen's envelope inputs: SC's Env.asArray.
	std::vector<V> envGenArray(const char* msg) const;
	// IEnvGen's: SC's Env.asArrayForInterpolation.
	std::vector<V> interpolationArray(Thread& th, const char* msg) const;
};

inline Env* asEnv(Arg v) { return v.o() ? dynamic_cast<Env*>(v.o()) : nullptr; }

#endif
