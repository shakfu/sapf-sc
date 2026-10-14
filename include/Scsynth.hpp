//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// OSC to scsynth: one this process runs, when the executable embeds it and $SAPF_SCSYNTH is unset,
// or else one over UDP at $SAPF_SCSYNTH (host:port), default 127.0.0.1:57110.

#ifndef SAPF_SCSYNTH_HPP
#define SAPF_SCSYNTH_HPP

#include <cstdint>
#include <string>
#include <vector>

struct OscArg {
	enum Type { Int, String, Blob, Float } type;
	int i;
	std::string s;   // String or Blob bytes
	float f = 0.f;
};

std::string oscMessage(const std::string& address, const std::vector<OscArg>& args);

// A bundle of messages at `seconds`: NTP time for a realtime server, time from the score's start
// for NRT.
std::string oscBundle(double seconds, const std::vector<std::string>& messages, bool realtime);

// A server that takes OSC packets in this process. The executable registers one; the interpreter
// library has none, so an SSP plugin can supply its own.
struct ScsynthServer {
	virtual ~ScsynthServer() = default;
	// As scsynthSendSync. Starts the server on first use.
	virtual std::string sendSync(const std::vector<std::string>& packets, int timeoutMs) = 0;
	virtual void send(const std::string& packet) = 0;
	virtual std::string address() = 0;
	// Renders an NRT score to `path`, a 32-bit float WAV of `channels` and exactly `frames` at
	// `sr`, in a non-realtime World of its own. "" or what went wrong.
	virtual std::string render(const std::string& score, const std::string& path, int channels, int64_t frames,
		double sr) = 0;
	// The server's time in seconds since 1970, which bundle times and the ola scheduler follow. The
	// wall clock by default; a server run by a host's audio callback counts its samples instead.
	virtual double now();
};
void setEmbeddedScsynth(ScsynthServer* server);

// As ScsynthServer::render, with the embedded server whether or not $SAPF_SCSYNTH is set.
std::string scsynthRender(const std::string& score, const std::string& path, int channels, int64_t frames, double sr);

// Sends `packets`, then /sync, and waits up to `timeoutMs` for /synced. Returns "" on
// success, else what went wrong: no reply, or the text of a /fail reply.
std::string scsynthSendSync(const std::vector<std::string>& packets, int timeoutMs = 1000);

// Sends without waiting for a reply.
void scsynthSend(const std::string& packet);

// The server address, for messages.
std::string scsynthAddress();

// The server's time: ScsynthServer::now, or the wall clock for a server over UDP.
double scsynthNow();

#endif
