//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "Modules.hpp"
#include "VM.hpp"
#include "Environment.hpp"
#include "Opcode.hpp"
#include "Parser.hpp"
#include "Completion.hpp"
#if SAPF_TREESITTER
#include "Highlight.hpp"
#endif
#include "Namespace.hpp"
#include "MultichannelExpansion.hpp"
#include "elapsedTime.hpp"
#include <algorithm>
#include <stdexcept>
#include <limits.h>
#include <chrono>
#ifdef _WIN32
#include "sapf/platform/WindowsCompat.hpp"
#endif

VM vm;

std::mutex gHelpMutex;
std::atomic<bool> gInterrupt{false};

std::atomic<int32_t> randSeedCounter = 77777;

uint64_t timeseed()
{
#ifdef _WIN32
	auto now = std::chrono::high_resolution_clock::now();
	auto duration = now.time_since_epoch();
	auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
	int32_t counter = ++randSeedCounter;
	return Hash64(micros / 1000000) + Hash64(micros % 1000000) + Hash64(counter);
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	int32_t counter = ++randSeedCounter;
	return Hash64(tv.tv_sec) + Hash64(tv.tv_usec) + Hash64(counter);
#endif
}


Thread::Thread()
    :rate(vm.ar), stackBase(0), localBase(0),
	mWorkspace(new GForm()),
    mScope(new Scope(mWorkspace)),
    parsingWhat(parsingWords),
    fromString(false),
#if USE_REPLXX
    replxx(nullptr),
    currentLine(nullptr),
#endif
    line(NULL)
{
	rgen.init(timeseed());
}

Thread::Thread(const Thread& inParent)
    :rate(inParent.rate), stackBase(0), localBase(0),
    mWorkspace(inParent.mWorkspace),
    mScope(inParent.mScope),
    mScopeVersion(inParent.mScopeVersion),
    parsingWhat(parsingWords),
    fromString(false),
#if USE_REPLXX
    replxx(nullptr),
    currentLine(nullptr),
#endif
    line(NULL)
{
	rgen.init(timeseed());
}

Thread::Thread(const Thread& inParent, P<Fun> const& inFun)
    :rate(vm.ar), stackBase(0), localBase(0),
    fun(inFun),
    mWorkspace(inParent.mWorkspace),
    mScope(inParent.mScope),
    mScopeVersion(inParent.mScopeVersion),
    parsingWhat(parsingWords),
    fromString(false),
#if USE_REPLXX
    replxx(nullptr),
    currentLine(nullptr),
#endif
    line(NULL)
{
	rgen.init(timeseed());
}

Thread::~Thread() {}

void Thread::flushBacktrace()
{
	if (!mBacktrace.empty()) post("%s", mBacktrace.c_str());
	mBacktrace.clear();
}

V Thread::lookupTop(Fun* f, Arg key)
{
	Scope* s = f->mDef->mScope();
	if (!s || s == mScope()) return mWorkspace->mustGet(*this, key);
	for (ScopeView& v : mOtherScopes)
		if (v.scope() == s) {
			if (v.version != s->version()) v.ws = s->current(), v.version = s->version();
			return v.ws->mustGet(*this, key);
		}
	mOtherScopes.push_back({s, s->current(), s->version()});
	return mOtherScopes.back().ws->mustGet(*this, key);
}

//////////////////////

static void inherit_(Thread& th, Prim* prim)
{
	V vparent = th.pop();
	P<Form> form = asParent(th, vparent);
	th.push(form);
}

P<Form> extendFormByOne(Thread& th, P<Form> const& parent, P<TableMap> const& tmap, Arg value)
{	
	P<Table> table = new Table(tmap);
	P<Form> form = new Form(table, parent);
	table->put(0, value);
	return form;
}

static void newForm_(Thread& th, Prim* prim)
{
	TableMap* tmap = (TableMap*)th.pop().o();
	size_t numArgs = tmap->mSize;
	
	V* args = &th.top() - numArgs + 1;
	
	P<Table> table = new Table(tmap);
	V vparent = args[-1];
	P<Form> parent = asParent(th, vparent);
	
	P<Form> form = new Form(table, parent);
	
	for (size_t i = 0; i < numArgs; ++i) {
		table->put(i, args[i]);
	}
	
	th.popn(numArgs+1);
	
	th.push(form);
}

static void newVList_(Thread& th, Prim* prim)
{
	size_t n = th.stackDepth();
	
	P<List> seq;
	if (n == 0) {
		seq = vm._nilv;
	} else {
		seq = new List(itemTypeV, n);
		V* ssp = &th.top() - n + 1;
		
		for (size_t i = 0; i < n; ++i) {
			seq->add(ssp[i]);
		}
	}
	
	th.popn(n);
	th.push(seq);		
}

static void newZList_(Thread& th, Prim* prim)
{
	size_t n = th.stackDepth();
	
	P<List> seq;
	if (n == 0) {
		seq = vm._nilz;
	} else {
		seq = new List(itemTypeZ, n);
		V* ssp = &th.top() - n + 1;
		
		for (size_t i = 0; i < n; ++i) {
			seq->add(ssp[i]);
		}
	}
	
	th.popn(n);
	
	th.push(seq);		
}

//////////////////////


VM::VM()
	:
	prelude_file(NULL),
	log_file(NULL),
	_ee(0),

	printLength(20),
	printDepth(8),

	ar(kDefaultSampleRate, kDefaultZBlockSize),
	kr(ar, kDefaultControlBlockSize),

	VblockSize(kDefaultVBlockSize),

#if COLLECT_MINFO
	totalRetains(0),
	totalReleases(0),
	totalObjectsAllocated(0),
	totalObjectsFreed(0),
	totalSignalGenerators(0),
	totalStreamGenerators(0)
#endif
{
	initElapsedTime();

	_ee = new Form(new Table(new TableMap(0)), NULL);

	builtins = new GTable();

	// add built in funs

	_nilz = new List(itemTypeZ);
	_nilv = new List(itemTypeV);

	_anilz = _nilz->mArray;
	_anilv = _nilv->mArray;

	newForm = new Prim(newForm_, 0., 0, 1, NULL, NULL);
	inherit = new Prim(inherit_, 0., 0, 1, NULL, NULL);

	newVList = new Prim(newVList_, 0., 0, 1, NULL, NULL);
	newZList = new Prim(newZList_, 0., 0, 1, NULL, NULL);
}

VM::~VM()
{
}

#if USE_REPLXX
static const char* getPrompt(int parsingWhat)
{
	switch (parsingWhat) {
		default:
		case parsingWords:  return "stac> ";
		case parsingString: return "\"stac> ";
		case parsingParens: return "(stac> ";
		case parsingLambda: return "\\stac> ";
		case parsingArray:  return "[stac> ";
		case parsingEnvir:  return "{stac> ";
	}
}
#endif

#if USE_REPLXX
// Tab completes the name at the cursor, NS.word included (Completion.hpp).
static void completeCallback(const char* input, replxx_completions* out, int* contextLen, void* th)
{
	std::string context = completionContext(input);
	*contextLen = (int)context.size();
	for (std::string const& c : completions(*(Thread*)th, context)) replxx_add_completion(out, c.c_str());
}

// Shows the stack effect of the word at the cursor, or of its only completion.
static void hintCallback(const char* input, replxx_hints* out, int* contextLen, ReplxxColor* color, void* th)
{
	std::string context = completionContext(input);
	if (context.empty()) return;
	Thread& t = *(Thread*)th;
	std::string word = context;
	std::string effect = stackEffect(t, word);
	if (effect.empty()) {
		std::vector<std::string> all = completions(t, context);
		if (all.size() != 1) return;
		word = all[0];
		effect = stackEffect(t, word);
	}
	*contextLen = (int)context.size();
	*color = REPLXX_COLOR_GRAY;
	replxx_add_hint(out, (word + (effect.empty() ? "" : "  " + effect)).c_str());
}
#endif

#if USE_REPLXX && SAPF_TREESITTER
// The terminal's 16 colours, so the user's theme applies.
static ReplxxColor colorOf(Hl kind)
{
	switch (kind) {
		case Hl::Comment:    return REPLXX_COLOR_GRAY;
		case Hl::String:     return REPLXX_COLOR_GREEN;
		case Hl::Symbol:     return REPLXX_COLOR_GREEN;
		case Hl::Number:     return REPLXX_COLOR_CYAN;
		case Hl::Keyword:    return replxx_color_bold(REPLXX_COLOR_MAGENTA);
		case Hl::Operator:   return REPLXX_COLOR_BROWN;
		case Hl::Definition: return replxx_color_bold(REPLXX_COLOR_BLUE);
		case Hl::Parameter:  return REPLXX_COLOR_BRIGHTCYAN;
		case Hl::Variable:   return REPLXX_COLOR_BRIGHTCYAN;
		case Hl::Bound:      return REPLXX_COLOR_BRIGHTCYAN;
		case Hl::Library:    return REPLXX_COLOR_BRIGHTBLUE;
		case Hl::UGen:       return REPLXX_COLOR_BRIGHTMAGENTA;
		case Hl::Type:       return REPLXX_COLOR_BROWN;
		case Hl::Module:     return REPLXX_COLOR_BLUE;
		case Hl::Property:   return REPLXX_COLOR_CYAN;
		case Hl::Method:     return REPLXX_COLOR_BRIGHTBLUE;
		case Hl::Unbound:    return REPLXX_COLOR_RED;
		default:             return REPLXX_COLOR_DEFAULT;
	}
}

// SGR codes as in LS_COLORS: 0, 30-37, 90-97, 39, 38;5;N, 1 (bold), 4 (underline). False on any other.
static bool parseSgr(std::string const& codes, ReplxxColor& color)
{
	ReplxxColor c = REPLXX_COLOR_DEFAULT;
	bool bold = false, underline = false;
	std::vector<int> n;
	for (size_t i = 0; i <= codes.size();) {
		size_t j = codes.find(';', i);
		if (j == std::string::npos) j = codes.size();
		std::string part = codes.substr(i, j - i);
		if (part.empty() || part.find_first_not_of("0123456789") != std::string::npos) return false;
		n.push_back(atoi(part.c_str()));
		i = j + 1;
	}
	for (size_t i = 0; i < n.size(); ++i) {
		if (n[i] >= 30 && n[i] <= 37) c = (ReplxxColor)(n[i] - 30);
		else if (n[i] >= 90 && n[i] <= 97) c = (ReplxxColor)(n[i] - 90 + 8);
		else if (n[i] == 39 || n[i] == 0) c = REPLXX_COLOR_DEFAULT;
		else if (n[i] == 1) bold = true;
		else if (n[i] == 4) underline = true;
		else if (n[i] == 38 && i + 2 < n.size() && n[i + 1] == 5 && n[i + 2] <= 255) {
			int x = n[i + 2];
			c = x < 16 ? (ReplxxColor)x
			  : x < 232 ? replxx_color_rgb666((x - 16) / 36, (x - 16) / 6 % 6, (x - 16) % 6)
			  : replxx_color_grayscale(x - 232);
			i += 2;
		} else return false;
	}
	if (bold) c = replxx_color_bold(c);
	if (underline) c = replxx_color_underline(c);
	color = c;
	return true;
}

// colorOf, overridden by STAC_COLORS: kind=SGR pairs separated by ':', such as "number=36:library=1;94".
static ReplxxColor const* palette()
{
	static const char* names[] = {"plain", "comment", "string", "number", "keyword", "operator", "bracket",
		"delimiter", "definition", "parameter", "type", "variable", "module", "symbol", "property", "method",
		"word", "library", "ugen", "bound", "unbound"};
	constexpr size_t count = sizeof names / sizeof *names;
	static_assert(count == (size_t)Hl::Unbound + 1, "a name per Hl kind");
	static std::vector<ReplxxColor> colors = [] {
		std::vector<ReplxxColor> c;
		for (size_t i = 0; i < count; ++i) c.push_back(colorOf((Hl)i));
		const char* spec = stacEnv("COLORS");
		std::string all = spec ? spec : "";
		for (size_t i = 0; i < all.size();) {
			size_t j = all.find(':', i);
			if (j == std::string::npos) j = all.size();
			std::string entry = all.substr(i, j - i);
			i = j + 1;
			if (entry.empty()) continue;
			size_t eq = entry.find('=');
			size_t k = std::find_if(names, names + count, [&](const char* n) { return entry.compare(0, eq, n) == 0 && eq == strlen(n); }) - names;
			if (eq == std::string::npos || k == count || !parseSgr(entry.substr(eq + 1), c[k]))
				post("STAC_COLORS: ignored '%s'\n", entry.c_str());
		}
		return c;
	}();
	return colors.data();
}

// replxx takes a colour per code point: each takes the kind of its first byte.
static void highlighterCallback(const char* input, ReplxxColor* colors, int size, void* th)
{
	std::string src(input);  // replxx_get_state refills the buffer input points to
	Thread& t = *(Thread*)th;
	std::vector<Hl> kinds = highlight(src, &t);
	// getLine's commands, not words
	for (const char* command : {"quit", ".."})
		if (src.rfind(command, 0) == 0) std::fill(kinds.begin(), kinds.begin() + strlen(command), Hl::Keyword);
	// The word being typed is not yet red: most of its prefixes are unbound.
	ReplxxState state;
	replxx_get_state(t.replxx, &state);
	size_t cursor = 0;
	for (int n = 0; cursor < src.size(); ++cursor)
		if (((unsigned char)src[cursor] & 0xC0) != 0x80 && n++ == state.cursorPosition) break;
	size_t a = cursor, b = cursor;
	while (a > 0 && kinds[a - 1] == Hl::Unbound) --a;
	while (b < kinds.size() && kinds[b] == Hl::Unbound) ++b;
	std::fill(kinds.begin() + a, kinds.begin() + b, Hl::Plain);
	int cp = 0;
	for (size_t i = 0; i < src.size() && cp < size; ++i)
		if (((unsigned char)src[i] & 0xC0) != 0x80) colors[cp++] = palette()[(size_t)kinds[i]];
}
#endif

void Thread::getLine()
{
	if (fromString) return;
#if USE_REPLXX
	// Free previous line if any (our malloc'd buffer with added newline)
	if (currentLine) {
		free(currentLine);
		currentLine = nullptr;
	}

	const char* prompt = getPrompt(parsingWhat);
	const char* input = replxx_input(replxx, prompt);

	if (!input || strncmp(input, "quit", 4)==0 || strncmp(input, "..", 2)==0) {
		line = NULL;
		throw errUserQuit;
	}

	// Add to history (replxx returns line without newline)
	size_t origLen = strlen(input);
	if (origLen > 0) {
		replxx_history_add(replxx, input);
		replxx_history_save(replxx, historyfilename);
		if (logfilename) {
			FILE* logfile = fopen(logfilename, "a");
			logTimestamp(logfile);
			fwrite(input, 1, origLen, logfile);
			fputc('\n', logfile);
			fclose(logfile);
		}
	}

	// Append newline to match libedit behavior (parser expects it)
	// Note: replxx manages input memory, so we make our own copy
	currentLine = (char*)malloc(origLen + 2);
	memcpy(currentLine, input, origLen);
	currentLine[origLen] = '\n';
	currentLine[origLen + 1] = '\0';

	line = currentLine;
	linelen = (int)(origLen + 1);
	linepos = 0;
#else
	// Fallback for platforms without replxx
	static char inputBuffer[4096];
	printf("%s", parsingWhat == parsingWords ? "stac> " : "...> ");
	fflush(stdout);
	if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
		line = NULL;
		throw errUserQuit;
	}
	line = inputBuffer;
	linelen = (int)strlen(line);
	linepos = 0;
	if (!line || strncmp(line, "quit", 4)==0 || strncmp(line, "..", 2)==0) {
		line = NULL;
		throw errUserQuit;
	}
#endif
}

void Thread::logTimestamp(FILE* logfile)
{
	timeval tv;
	gettimeofday(&tv, NULL);
	if (previousTimeStamp == 0 || tv.tv_sec - previousTimeStamp > 3600) {
		previousTimeStamp = tv.tv_sec;
		char date[32];
		time_t secs = static_cast<time_t>(tv.tv_sec);
		ctime_r(&secs, date);
		fprintf(logfile, ";;;;;;;; %s", date);
		fflush(logfile);
	}
}

char Thread::getc() { 
	if (fromString) {
		if (line == NULL) return 0;
		return line[linepos++];
	} else {
		while (1) {	
			if (line == NULL) {
				getLine();
				if (line == NULL || linelen == 0) return 0;
			} else if (line[linepos] == 0) {
				if (parsingWhat == parsingWords) {
					return 0;
				} else {
					getLine();
					if (linelen == 0 || strcmp(line, "\n") == 0) {
						line = NULL;
						continue;
					}
					if (line == NULL || linelen == 0) return 0;
				}
			} else {
				return line[linepos++];
			}
		}
	}
	return 0; // never gets here, but compiler too dumb to figure this out.
}

void Thread::repl(FILE* infile, const char* inLogfilename)
{
	Thread& th = *this;

	logfilename = inLogfilename;

	previousTimeStamp = 0;

#if USE_REPLXX
	// Initialize replxx
	replxx = replxx_init();

	// Set up history file path; legacyHistory is the pre-rename default, read once if the new file is absent
	char legacyHistory[PATH_MAX] = "";
	const char* envHistoryFileName = stacEnv("HISTORY");
	if (envHistoryFileName) {
		snprintf(historyfilename, PATH_MAX, "%s", envHistoryFileName);
	} else {
		const char* homeDir = getenv("HOME");
#ifdef _WIN32
		if (!homeDir) homeDir = getenv("USERPROFILE");
#endif
		if (homeDir) {
			snprintf(historyfilename, PATH_MAX, "%s/stac-history.txt", homeDir);
			snprintf(legacyHistory, PATH_MAX, "%s/sapf-history.txt", homeDir);
		} else {
			snprintf(historyfilename, PATH_MAX, "stac-history.txt");
		}
	}

	// Configure replxx
	replxx_set_max_history_size(replxx, 800);
	replxx_set_unique_history(replxx, 1);
	replxx_set_completion_callback(replxx, completeCallback, this);
	replxx_set_hint_callback(replxx, hintCallback, this);
#if SAPF_TREESITTER
	const char* noColor = getenv("NO_COLOR");  // no-color.org: set and not empty
	if (!noColor || !*noColor) replxx_set_highlighter_callback(replxx, highlighterCallback, this);
#endif
	if (replxx_history_load(replxx, historyfilename) != 0 && legacyHistory[0])
		replxx_history_load(replxx, legacyHistory);
#endif

	fflush(infile);
	bool running = true;

	post("Type 'helpall' to get a list of all built-in functions.\n");
	post("Type 'quit' to quit.\n");
	
	do {
		try {
			if (stackDepth()) {
				printStack();
				post("\n");
			}
		} catch (int err) {
			if (err <= -1000 && err > -1000 - kNumErrors) {
				post("\nerror: %s\n", errString[-1000 - err]);
			} else {
				post("\nerror: %d\n", err);
			}
		} catch (std::bad_alloc& xerr) {
			post("\nnot enough memory\n");
		} catch (...) {
			post("\nunknown error\n");
		}
				
		try {
			// PARSE
			{
				P<Fun> compiledFun;
				if (compile(NULL, compiledFun, true)) {
				// EVAL
					compiledFun->runREPL(th);
				}
				th.publishWorkspace();
			}
		} catch (V& v) {
			flushBacktrace();
            post("error: ");
            v.print(th);
			post("\n");
		} catch (int err) {
			flushBacktrace();
			if (err == errUserQuit) {
				post("good bye\n");
				running = false;
			} else if (err <= -1000 && err > -1000 - kNumErrors) {
				post("error: %s\n", errString[-1000 - err]);
			} else {
				post("error: %d\n", err);
			}
		} catch (std::bad_alloc& xerr) {
			post("not enough memory\n");
		} catch (...) {
			post("unknown error\n");
		}

	} while (running);

#if USE_REPLXX
	// Save history and clean up
	replxx_history_save(replxx, historyfilename);
	if (currentLine) {
		free(currentLine);  // We use malloc to add newline
		currentLine = nullptr;
	}
	replxx_end(replxx);
	replxx = nullptr;
#endif
}


template <typename T>
class AutoFree
{
	T* p;
public:
	AutoFree(T* _p) : p(_p) {}
	~AutoFree() { free(p); }
	
	T* operator()() { return p; }
	T* operator*() { return p; }
	T* operator->() { return p; }
};

void loadFile(Thread& th, const char* filename)
{
    post("loading file '%s'\n", filename);
	LoadingFile loading(filename);
	FILE* f = fopen(filename, "r");
	if (!f) {
		post("could not open '%s'\n", filename);
		return;
	}

	fseek(f, 0, SEEK_END);
	int64_t fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	AutoFree<char> buf = (char*)malloc(fileSize + 1);
	const char* p = buf();
	fread(buf(), 1, fileSize, f);
	buf()[fileSize - 1] = 0;
		
	try {
		{
			P<Fun> compiledFun;
			if (th.compile(p, compiledFun, true)) {
				post("compiled OK.\n");
				compiledFun->run(th);
				th.publishWorkspace();
				post("done loading file\n");
			}
		}
    } catch (V& v) {
		th.flushBacktrace();
        post("error: ");
        v.print(th);
        post("\n");
	} catch (int err) {
		th.flushBacktrace();
		if (err <= -1000 && err > -1000 - kNumErrors) {
			post("error: %s\n", errString[-1000 - err]);
		} else {
			post("error: %d\n", err);
		}
	} catch (std::bad_alloc& xerr) {
		post("not enough memory\n");
	} catch (...) {
		th.flushBacktrace();
		post("unknown error\n");
	}
}

void Thread::printStack()
{
	bool between = false;
	size_t n = stackDepth();
	for (size_t i = 0; i < n; ++i) {
		V* s = &stack[stackBase+i];
		if (between) post(" ");
		else between = true;
		std::string cppstring;
		s->print(*this, cppstring);
		post("%s", cppstring.c_str());
	}
}

void Thread::printLocals()
{
	bool between = false;
	size_t n = numLocals();
	for (size_t i = 0; i < n; ++i) {
		V* s = &local[localBase+i];
		if (between) post(" ");
		else between = true;
		std::string cppstring;
		s->print(*this, cppstring);
		post("%s", cppstring.c_str());
	}
}

void VM::setSampleRate(double inSampleRate)
{
	ar = Rate(inSampleRate, ar.blockSize);
	kr = Rate(ar, kDefaultControlBlockSize);
}

V VM::def(Arg key, Arg value)
{
	if (defNamespace) {
		defNamespace->mWords[((String*)key.o())->s] = value;
		return value;
	}
	builtins->putImpure(key, value); 
    V dummy;
    assert(builtins->getInner(key, dummy));
	return value;
}

std::string VM::helpName(const char* name) const
{
	return defNamespace ? defNamespace->mName + "." + name : std::string(name);
}

V VM::def(const char* name, Arg value)
{
	def(V(getsym(name)), value);
	return value;
}

V VM::def(const char* name, int takes, int leaves, PrimFun pf, const char* help, Arg value, bool setNoEach)
{
	V aPrim = new Prim(pf, value, takes, leaves, name, help);
	def(name, aPrim);
	
	if (setNoEach) aPrim.SetNoEachOps();
	
	if (help)
		addBifHelp(helpName(name).c_str(), aPrim.GetAutoMapMask(), help);
		
	return aPrim;
}

V VM::defmcx(const char* name, int numArgs, PrimFun pf, const char* help, Arg value)
{
	V aPrim = new Prim(pf, value, numArgs, 1, name, help);
	aPrim = mcx(numArgs, aPrim, name, help);
	def(name, aPrim);
		
	addBifHelp(helpName(name).c_str(), aPrim.GetAutoMapMask(), help);
	return aPrim;
}

V VM::defautomap(const char* name, const char* mask, PrimFun pf, const char* help, Arg value)
{
	int numArgs = (int)strlen(mask);
	V aPrim = new Prim(pf, value, numArgs, 1, name, help);
	aPrim = automap(mask, numArgs, aPrim, name, help);
	def(name, aPrim);
		
	addBifHelp(helpName(name).c_str(), aPrim.GetAutoMapMask(), help);
	return aPrim;
}


#pragma mark STACK

#define POPREFTYPEDEF(TYPE) \
P<TYPE> Thread::pop##TYPE(const char* msg) \
{ \
	V v = pop(); \
    ApplyIfFun(v); \
	if (!v.is##TYPE()) wrongType(msg, #TYPE, v); \
	return reinterpret_cast<TYPE*> (v.o()); \
}

#define POPTYPEDEF(TYPE) \
P<TYPE> Thread::pop##TYPE(const char* msg) \
{ \
	V v = pop().deref(); \
    ApplyIfFun(v); \
	if (!v.is##TYPE()) wrongType(msg, #TYPE, v); \
	return reinterpret_cast<TYPE*> (v.o()); \
}

#define POPFUNTYPEDEF(TYPE) \
P<TYPE> Thread::pop##TYPE(const char* msg) \
{ \
	V v = pop().deref(); \
	if (!v.is##TYPE()) wrongType(msg, #TYPE, v); \
	return reinterpret_cast<TYPE*> (v.o()); \
}

POPREFTYPEDEF(Ref);
POPTYPEDEF(ZRef);
POPTYPEDEF(String);
POPTYPEDEF(List);
POPFUNTYPEDEF(Fun);
POPTYPEDEF(Form);

void Thread::ApplyIfFun(V& v)
{
    if (v.isFunOrPrim()) {
        SaveStack ss(*this);
        v.apply(*this);
        v = pop();
    }
}


V Thread::popZInList(const char* msg)
{
	V p = pop();
	if (p.isRef()) p = p.deref();
    ApplyIfFun(p);
	if (!p.isZIn() && !p.isVList()) {
		wrongType(msg, "Real or Signal or List of Reals or Signals", p);
	}
	return p;
}

V Thread::popZIn(const char* msg)
{
	V p = pop();
	if (p.isRef() || p.isZRef()) p = p.deref();
    ApplyIfFun(p);
	if (!p.isZIn()) {
		wrongType(msg, "Real or Signal", p);
	}
	return p;
}

V Thread::popValue()
{
	V p = pop().deref();
    ApplyIfFun(p);
	return p;
}

P<List> Thread::popVList(const char* msg)
{
	V v = pop().deref();
    ApplyIfFun(v);
	if (!v.isVList()) {
		wrongType(msg, "Stream", v);
	}
	return (List*)v.o();
}

P<List> Thread::popZList(const char* msg)
{
	V v = pop().deref();
    ApplyIfFun(v);
	if (!v.isZList()) {
		wrongType(msg, "Signal", v);
	}
	return (List*)v.o();
}

int64_t Thread::popInt(const char* msg)
{
	V v = pop().deref();
    ApplyIfFun(v);
	if (!v.isReal()) wrongType(msg, "Real", v);
	
	double f = v.f;
	int64_t i;
	if (f >= (double)LLONG_MAX) i = LLONG_MAX;
	else if (f <= (double)LLONG_MIN) i = LLONG_MIN;
	else i = (int64_t)f;
	return i;
}

double Thread::popFloat(const char* msg)
{
	V v = pop().deref();
    ApplyIfFun(v);
	if (!v.isReal()) wrongType(msg, "Float", v);
	return v.f;
}

void Thread::tuck(size_t n, Arg v)
{
	stack.push_back(V(0.));
	V* sp = &stack.back();
	
	for (size_t i = 0; i < n; ++i)
		sp[-i] = sp[-i-1];
	
	sp[-n] = v;
}

///////////////////////////////////////

bool Thread::compile(const char* inString, P<Fun>& compiledFun, bool inTopLevel)
{	
	Thread& th = *this;
	SaveCompileScope scs(th);
	if (inTopLevel) {
		mCompileScope = new TopCompileScope();
	} else {
		mCompileScope = new InnerCompileScope(new TopCompileScope());
	}
	
	P<Code> code;
	setParseString(inString);
	getLine();
	bool ok = parseElems(th, code);
	if (!ok || !code) {
		post("parse error. %d\n", ok);
		return false;
	}
		
	compiledFun = new Fun(*this, new FunDef(*this, code, 0, th.mCompileScope->numLocals(), th.mCompileScope->numVars(), NULL));
	
	return true;
}

int TopCompileScope::directLookup(Thread& th, P<String> const& inName, size_t& outIndex, V& outBuiltIn)
{
	int scope = CompileScope::directLookup(th, inName, outIndex, outBuiltIn);
	if (scope != scopeUndefined) return scope;

	for (size_t i = 0; i < mWorkspaceVars.size(); ++i) {
		if (mWorkspaceVars[i].mName() == inName()) {
			return scopeWorkspace;
		}
	}
	
	V value;
	if (th.mWorkspace->get(th, inName(), value)) {
		return scopeWorkspace;
	} else if (vm.builtins->get(th, inName, value)) {
		outBuiltIn = value;
		return scopeBuiltIn;
	}

	return scopeUndefined;
}


int CompileScope::directLookup(Thread& th, P<String> const& inName, size_t& outIndex, V& outBuiltIn)
{
	for (size_t i = 0; i < mLocals.size(); ++i) {
		if (mLocals[i].mName() == inName()) {
			outIndex = i;
			return scopeLocal;
		}
	}
	for (size_t i = 0; i < mVars.size(); ++i) {
		if (mVars[i].mName() == inName()) {
			outIndex = i;
			return scopeFunVar;
		}
	}
	
	return scopeUndefined;
}


int TopCompileScope::indirectLookup(Thread& th, P<String> const& inName, size_t& outIndex, V& outBuiltIn)
{
	return directLookup(th, inName, outIndex, outBuiltIn);
}

int InnerCompileScope::indirectLookup(Thread& th, P<String> const& inName, size_t& outIndex, V& outBuiltIn)
{
	int scope = directLookup(th, inName, outIndex, outBuiltIn);
	if (scope != scopeUndefined) 
		return scope;

	size_t outerIndex;
	scope = mNext->indirectLookup(th, inName, outerIndex, outBuiltIn);
	if (scope == scopeUndefined) 
		return scopeUndefined;
	
	if (scope == scopeLocal || scope == scopeFunVar) {
		VarDef def;
		def.mName = inName;
		def.mIndex = mVars.size();
		def.mFromScope = scope;
		def.mFromIndex = outerIndex;
		outIndex = mVars.size();
		mVars.push_back(def);
		return scopeFunVar;
	}
	
	return scope;
}

int TopCompileScope::bindVar(Thread& th, P<String> const& inName, size_t& outIndex)
{
	V v;
	int scope = directLookup(th, inName, outIndex, v);
	if (scope != scopeUndefined && scope != scopeBuiltIn) 
		return scope; // already defined
		
	WorkspaceDef def;
	def.mName = inName;
	mWorkspaceVars.push_back(def);

	return scopeWorkspace;
}

int CompileScope::innerBindVar(Thread& th, P<String> const& inName, size_t& outIndex)
{
	V v;
	int scope = directLookup(th, inName, outIndex, v);
	if (scope == scopeFunVar) {
		post("Name %s is already in use in this scope as a free variable.\n", inName->cstr());
		throw errSyntax;
	}
	
	if (scope == scopeUndefined) {		
		LocalDef def;
		def.mName = inName;
		outIndex = def.mIndex = mLocals.size();
		mLocals.push_back(def);
	}
	return scopeLocal;
}

int InnerCompileScope::bindVar(Thread& th, P<String> const& inName, size_t& outIndex)
{
	return innerBindVar(th, inName, outIndex);
}

CompileScope* ParenCompileScope::nextNonParen() const
{
	CompileScope* scope = mNext();
	while (scope->isParen()) {
		scope = scope->mNext();
	}
	return scope;
}

int ParenCompileScope::directLookup(Thread& th, P<String> const& inName, size_t& outIndex, V& outBuiltIn)
{
	return mNext->directLookup(th, inName, outIndex, outBuiltIn);
}

int ParenCompileScope::indirectLookup(Thread& th, P<String> const& inName, size_t& outIndex, V& outBuiltIn)
{
	return mNext->indirectLookup(th, inName, outIndex, outBuiltIn);
}

int ParenCompileScope::innerBindVar(Thread& th, P<String> const& inName, size_t& outIndex)
{
	return mNext->innerBindVar(th, inName, outIndex);
}

int ParenCompileScope::bindVar(Thread& th, P<String> const& inName, size_t& outIndex)
{
	return mNext->innerBindVar(th, inName, outIndex);
}

//////////////////////




