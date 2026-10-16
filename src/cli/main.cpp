#include "sapf/Engine.hpp"
#include "sapf/ReplRunner.hpp"
#include "VM.hpp"
#include "CLI11.hpp"
#include <cstdio>
#ifdef SAPF_EMBED_SCSYNTH
#include "EmbeddedServer.hpp"
#endif

int main(int argc, const char * argv[])
{
	SapfEngineConfig config;
	std::string inputFile;
	bool interactive = false;
	bool quiet = false;
	bool words = false;

	CLI::App app{"stac - a concatenative language for live coding SuperCollider"};
	app.add_option("-r,--rate", config.sampleRate, "Sample rate for list timing ops such as T (1000-768000)")
		->check(CLI::Range(1000.0, 768000.0));
	app.add_option("-p,--prelude", config.preludeFiles,
		"Prelude file to load, in place of $STAC_PRELUDE; repeat to load several in order")
		->allow_extra_args(false);
	app.add_flag("-i,--interactive", interactive, "Interactive mode (enter REPL after running file)");
	app.add_flag("-q,--quiet", quiet, "Quiet mode (suppress banner)");
	app.add_flag("--words", words, "Print every word, after the preludes, as JSON, and exit");
	app.add_option("file", inputFile, "Input file to load and execute");

	CLI11_PARSE(app, argc, argv);

#ifdef SAPF_EMBED_SCSYNTH
	registerEmbeddedScsynth();
#endif

	// stdout carries only the JSON; the preludes' messages go to stderr
	if (words) setPostHandler([](const char* s) { fputs(s, stderr); });

	if (!quiet && !words) {
		post("------------------------------------------------\n");
		post("A tool for the expression of sound as pure form.\n");
		post("------------------------------------------------\n");
		post("--- version %s\n", SapfGetVersionString());
	}

	SapfEngine& engine = GetSapfEngine();
	engine.configure(config);
	engine.initialize();

	Thread th;
	engine.loadPrelude(th);
	if (words) {
		fputs(wordsJson().c_str(), stdout);
		return 0;
	}

	if (!inputFile.empty()) {
		loadFile(th, inputFile.c_str());

		if (!interactive) {
#ifdef SAPF_EMBED_SCSYNTH
			waitForEmbeddedScsynth();  // what the file plays, until it ends
#endif
			return 0;
		}
	}

	// Enter REPL
	RunSapfRepl(th, engine.logFile());

	return 0;
}
