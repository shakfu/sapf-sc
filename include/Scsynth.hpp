//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// OSC over UDP to a running scsynth at $SAPF_SCSYNTH (host:port), default 127.0.0.1:57110.

#ifndef SAPF_SCSYNTH_HPP
#define SAPF_SCSYNTH_HPP

#include <string>
#include <vector>

struct OscArg {
	enum Type { Int, String, Blob } type;
	int i;
	std::string s;   // String or Blob bytes
};

std::string oscMessage(const std::string& address, const std::vector<OscArg>& args);

// Sends `packets`, then /sync, and waits up to `timeoutMs` for /synced. Returns "" on
// success, else what went wrong: no reply, or the text of a /fail reply.
std::string scsynthSendSync(const std::vector<std::string>& packets, int timeoutMs = 1000);

// Sends without waiting for a reply.
void scsynthSend(const std::string& packet);

// The server address, for messages.
std::string scsynthAddress();

#endif
