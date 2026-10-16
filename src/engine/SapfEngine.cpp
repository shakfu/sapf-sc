#include "sapf/Engine.hpp"
#include "Environment.hpp"
#include "Namespace.hpp"
#include "sapf/platform/Platform.hpp"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include "sapf/platform/WindowsCompat.hpp"
#else
#include <unistd.h>
#endif


extern void AddCoreOps();
extern void AddMathOps();
extern void AddStreamOps();
extern void AddSetOps();
extern void AddRandomOps();
extern void AddSCUGenOps();
extern void AddSpecUGenOps();
extern void AddEnvOps();
extern void AddDefOps();
extern void AddResourceOps();
extern void AddPatternOps();
extern void AddTextureOps();
extern void AddWavetableOps();
extern void AddSampleOps();

namespace {

const char* gVersionString = "0.1.22";


} // namespace

SapfEngine::SapfEngine()
	: config_()
	, initialized_(false)
{
}

void SapfEngine::configure(const SapfEngineConfig& config)
{
	config_ = config;
	if (config.sampleRate > 0.) {
		vm.setSampleRate(config.sampleRate);
	}
	if (!config_.preludeFiles.empty()) {
		vm.prelude_file = config_.preludeFiles[0].c_str();
	}
	if (config.logFile) {
		vm.log_file = config.logFile;
	}
}

void SapfEngine::initialize()
{
	if (initialized_) {
		return;
	}
	registerBuiltins();
	configureLogFile();
	initialized_ = true;
}

void SapfEngine::registerBuiltins()
{
	vm.addBifHelp("Argument Automapping legend:");
	vm.addBifHelp("   a - as is. argument is not automapped.");
	vm.addBifHelp("   z - argument is expected to be a signal or scalar, streams are auto mapped.");
	vm.addBifHelp("   k - argument is expected to be a scalar, signals and streams are automapped.");
	vm.addBifHelp("");

	AddCoreOps();
	AddMathOps();
	AddStreamOps();
	AddRandomOps();
	AddSCUGenOps();
	AddSpecUGenOps();
	AddEnvOps();
	AddDefOps();
	AddResourceOps();
	AddPatternOps();
	AddTextureOps();
	AddWavetableOps();
	AddSampleOps();
	AddSetOps();
	AddStacNamespace();
}

void SapfEngine::configureLogFile()
{
	if (vm.log_file) {
		return;
	}
	vm.log_file = stacEnv("LOG");
	if (!vm.log_file) {
		const char* home_dir = getenv("HOME");
		static char logfilename[PATH_MAX];
		snprintf(logfilename, PATH_MAX, "%s/sapf-log.txt", home_dir);
		vm.log_file = logfilename;
	}
}


void SapfEngine::loadPrelude(Thread& th) const
{
	if (!config_.preludeFiles.empty()) {
		for (auto& file : config_.preludeFiles) loadFile(th, file.c_str());
		return;
	}
	if (!vm.prelude_file) {
		vm.prelude_file = stacEnv("PRELUDE");
	}
	if (vm.prelude_file) {
		loadFile(th, vm.prelude_file);
	}
}

const char* SapfEngine::versionString() const
{
	return gVersionString;
}

const char* SapfEngine::logFile() const
{
	return vm.log_file;
}

SapfEngine& GetSapfEngine()
{
	static SapfEngine engine;
	return engine;
}

const char* SapfGetVersionString()
{
	return gVersionString;
}
