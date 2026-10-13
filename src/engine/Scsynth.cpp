//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "Scsynth.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifndef _WIN32
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace {

void pad(std::string& out) { out.append((4 - out.size() % 4) % 4, '\0'); }

void putInt(std::string& out, int32_t x)
{
	uint32_t u = (uint32_t)x;
	for (int shift = 24; shift >= 0; shift -= 8) out += (char)(uint8_t)(u >> shift);
}

void putString(std::string& out, const std::string& s)
{
	out += s;
	out += '\0';
	pad(out);
}

// Reads the OSC string at `pos`, advancing past its padding.
std::string getString(const std::string& in, size_t& pos)
{
	size_t end = in.find('\0', pos);
	if (end == std::string::npos) end = in.size();
	std::string s = in.substr(pos, end - pos);
	pos = (end + 4) & ~(size_t)3;
	return s;
}

void splitAddress(std::string& host, std::string& port)
{
	const char* env = getenv("SAPF_SCSYNTH");
	std::string addr = (env && *env) ? env : "127.0.0.1:57110";
	size_t colon = addr.rfind(':');
	host = colon == std::string::npos ? addr : addr.substr(0, colon);
	port = colon == std::string::npos ? "57110" : addr.substr(colon + 1);
}

} // namespace

std::string oscMessage(const std::string& address, const std::vector<OscArg>& args)
{
	std::string tags = ",";
	std::string data;
	for (const OscArg& a : args) {
		switch (a.type) {
			case OscArg::Int : tags += 'i'; putInt(data, a.i); break;
			case OscArg::String : tags += 's'; putString(data, a.s); break;
			case OscArg::Blob : tags += 'b'; putInt(data, (int32_t)a.s.size()); data += a.s; pad(data); break;
		}
	}
	std::string out;
	putString(out, address);
	putString(out, tags);
	return out + data;
}

std::string scsynthAddress()
{
	std::string host, port;
	splitAddress(host, port);
	return host + ":" + port;
}

#ifdef _WIN32

std::string scsynthSendSync(const std::vector<std::string>&, int) { return "not supported on Windows"; }
void scsynthSend(const std::string&) {}

#else

namespace {

// A UDP socket connected to scsynth, so replies come back to it.
struct Socket {
	int fd = -1;
	std::string error;

	Socket()
	{
		std::string host, port;
		splitAddress(host, port);
		addrinfo hints = {};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		addrinfo* res = nullptr;
		if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0 || !res) {
			error = "cannot resolve " + host;
			return;
		}
		fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (fd < 0 || connect(fd, res->ai_addr, res->ai_addrlen) != 0) error = "cannot open a UDP socket";
		freeaddrinfo(res);
	}
	~Socket() { if (fd >= 0) close(fd); }

	bool send(const std::string& packet) { return ::send(fd, packet.data(), packet.size(), 0) == (ssize_t)packet.size(); }
};

} // namespace

std::string scsynthSendSync(const std::vector<std::string>& packets, int timeoutMs)
{
	Socket sock;
	if (!sock.error.empty()) return sock.error;
	const int syncID = 7301;
	for (const std::string& p : packets)
		if (!sock.send(p)) return "packet not sent; a SynthDef over about 64 KB does not fit in UDP";
	sock.send(oscMessage("/sync", {{OscArg::Int, syncID, ""}}));

	timeval tv = {timeoutMs / 1000, (timeoutMs % 1000) * 1000};
	setsockopt(sock.fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	char buf[8192];
	for (;;) {
		ssize_t n = recv(sock.fd, buf, sizeof(buf), 0);
		if (n <= 0) return "no reply";
		std::string reply(buf, (size_t)n);
		size_t pos = 0;
		std::string address = getString(reply, pos);
		if (address == "/synced") return "";
		if (address == "/fail") {
			getString(reply, pos); // type tags
			std::string command = getString(reply, pos);
			return command + ": " + getString(reply, pos);
		}
	}
}

void scsynthSend(const std::string& packet)
{
	Socket sock;
	if (sock.error.empty()) sock.send(packet);
}

#endif
