#pragma once

#include "VM.hpp"

#include <functional>
#include <string>
#include <vector>

// Sends post's output, such as errors and printed values, to `handler` in place of stdout; an empty
// handler restores stdout. Set it before the interpreter runs: ola's threads also post.
void setPostHandler(std::function<void(const char*)> handler);

struct SapfEngineConfig {
	double sampleRate = kDefaultSampleRate;
	std::vector<std::string> preludeFiles; // loaded in order, in place of $STAC_PRELUDE
	const char* logFile = nullptr;
};

class SapfEngine {
public:
	SapfEngine();

	void configure(const SapfEngineConfig& config);
	void initialize();
	void loadPrelude(Thread& th) const;

	const char* versionString() const;
	const char* logFile() const;

private:
	void configureLogFile();
	void registerBuiltins();

	SapfEngineConfig config_;
	bool initialized_;
};

SapfEngine& GetSapfEngine();
const char* SapfGetVersionString();
