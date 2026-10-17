// stac as a CLAP instrument: scsynth runs in the plugin, driven by the host's audio callback, and
// code comes from the ImGui editor or from UDP datagrams to 127.0.0.1.
//
// Threads:
//   main      - CLAP lifecycle, editor, state
//   audio     - process() -> renderHostDrivenScsynth (never blocks)
//   evaluator - owns the stac Thread; compiles and runs code from a FIFO queue
//   udp       - receives datagrams, waits for the evaluator, replies with the output
//
// The interpreter and scsynth are one per process, so only one plugin instance can exist at a time.

#include "editor.hpp"
#include "udp_server.hpp"

#include "EmbeddedServer.hpp"
#include "Environment.hpp"
#include "Patterns.hpp"
#include "Scsynth.hpp"
#include "Texture.hpp"
#include "VM.hpp"
#include "sapf/Engine.hpp"

#include <clap/clap.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>

namespace {

const uint16_t kDefaultUdpPort = 57150;
const uint32_t kChannels = 2;
const size_t kMaxPendingLog = 1 << 20;
const auto kUdpReplyTimeout = std::chrono::seconds(10);

std::atomic<bool> gInstanceExists{false};
std::string gResourceDir; // UGens in plugins/, and prelude.stac, std.stac and sapf.stac

clap_plugin_descriptor_t gDescriptor = {
	CLAP_VERSION_INIT,
	"com.github.shakfu.stac",
	"stac",
	"shakfu",
	"https://github.com/shakfu/stac",
	"",
	"",
	"", // version: set in entry_init
	"Concatenative live coding for SuperCollider",
	nullptr, // features: set below
};

const char* const kFeatures[] = {
	CLAP_PLUGIN_FEATURE_INSTRUMENT,
	CLAP_PLUGIN_FEATURE_SYNTHESIZER,
	CLAP_PLUGIN_FEATURE_STEREO,
	nullptr,
};

uint16_t ConfiguredUdpPort(bool* fromEnv)
{
	const char* env = stacEnv("CLAP_UDP_PORT");
	*fromEnv = env != nullptr;
	if (!env) return kDefaultUdpPort;
	long port = strtol(env, nullptr, 10);
	return (port > 0 && port < 65536) ? (uint16_t)port : kDefaultUdpPort;
}

// The bundle's Contents/Resources on macOS, from the bundle or its binary; elsewhere the folder
// `stac` beside the .clap file.
std::string ResourceDir(const char* pluginPath)
{
	namespace fs = std::filesystem;
	fs::path path(pluginPath);
#ifdef __APPLE__
	std::error_code ec;
	if (fs::is_regular_file(path, ec)) return (path.parent_path().parent_path() / "Resources").string();
	return (path / "Contents" / "Resources").string();
#else
	return (path.parent_path() / "stac").string();
#endif
}

// Runs stac code on one thread in submission order. Output arrives via the post handler.
class Evaluator {
public:
	using Reply = std::shared_ptr<std::promise<std::string>>;

	explicit Evaluator(std::function<void(std::string*)> setCapture) : setCapture_(std::move(setCapture))
	{
		thread_ = std::thread([this] { loop(); });
	}

	~Evaluator()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			quit_ = true;
		}
		cv_.notify_one();
		thread_.join(); // blocks until the current evaluation returns
	}

	void eval(std::string code, Reply reply = nullptr) { push({std::move(code), 0., std::move(reply)}); }
	void setSampleRate(double rate) { push({std::string(), rate, nullptr}); }
	std::thread::id threadId() const { return thread_.get_id(); }

private:
	struct Job {
		std::string code;
		double sampleRate; // > 0: rate change job
		Reply reply;
	};

	void push(Job job)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			jobs_.push_back(std::move(job));
		}
		cv_.notify_one();
	}

	void loop()
	{
		Thread th;
		GetSapfEngine().loadPrelude(th);
		for (;;) {
			Job job;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				cv_.wait(lock, [this] { return quit_ || !jobs_.empty(); });
				if (quit_) return;
				job = std::move(jobs_.front());
				jobs_.pop_front();
			}
			if (job.sampleRate > 0.) {
				vm.setSampleRate(job.sampleRate);
				th.rate = vm.ar;
				continue;
			}
			std::string output;
			if (job.reply) setCapture_(&output);
			run(th, job.code);
			if (job.reply) {
				setCapture_(nullptr);
				job.reply->set_value(std::move(output));
			}
		}
	}

	// As a REPL line: compile, run, publish the workspace, print the stack.
	static void run(Thread& th, const std::string& code)
	{
		Report(th, [&] {
			P<Fun> fun;
			if (th.compile(code.c_str(), fun, true)) fun->runREPL(th);
			th.publishWorkspace();
		});
		Report(th, [&] {
			if (th.stackDepth()) {
				th.printStack();
				post("\n");
			}
		});
	}

	template <typename F>
	static void Report(Thread& th, F body)
	{
		try {
			body();
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
		} catch (std::bad_alloc&) {
			post("not enough memory\n");
		} catch (...) {
			post("unknown error\n");
		}
	}

	std::function<void(std::string*)> setCapture_;
	std::mutex mutex_;
	std::condition_variable cv_;
	std::deque<Job> jobs_;
	bool quit_ = false;
	std::thread thread_;
};

class StacPlugin {
public:
	explicit StacPlugin(const clap_host_t* host) : host_(host)
	{
		plugin_.desc = &gDescriptor;
		plugin_.plugin_data = this;
		plugin_.init = [](const clap_plugin_t* p) { return self(p)->init(); };
		plugin_.destroy = [](const clap_plugin_t* p) { delete self(p); };
		plugin_.activate = [](const clap_plugin_t* p, double rate, uint32_t, uint32_t) { return self(p)->activate(rate); };
		plugin_.deactivate = [](const clap_plugin_t*) { Deactivate(); };
		plugin_.start_processing = [](const clap_plugin_t*) { return true; };
		plugin_.stop_processing = [](const clap_plugin_t*) {};
		plugin_.reset = [](const clap_plugin_t*) {};
		plugin_.process = [](const clap_plugin_t*, const clap_process_t* process) { return Process(process); };
		plugin_.get_extension = [](const clap_plugin_t* p, const char* id) { return self(p)->extension(id); };
		plugin_.on_main_thread = [](const clap_plugin_t*) {};
	}

	~StacPlugin()
	{
		udp_.stop();
		destroyEditor();
		evaluator_.reset();
		Deactivate();
		setPostHandler(nullptr);
		gInstanceExists = false;
	}

	const clap_plugin_t* clapPlugin() const { return &plugin_; }

private:
	static StacPlugin* self(const clap_plugin_t* p) { return static_cast<StacPlugin*>(p->plugin_data); }

	// Players first, so none sends to the stopped World; stopping it frees the synths.
	static void Deactivate()
	{
		stopTextures();
		stopEventStreams();
		stopHostDrivenScsynth();
	}

	bool init()
	{
		registerHostDrivenScsynth(gResourceDir + "/plugins");
		// $STAC_PRELUDE, else the bundled prelude; imports also find the bundled modules.
		SapfEngineConfig config;
		if (!stacEnv("PRELUDE")) config.preludeFiles = {gResourceDir + "/prelude.stac"};
		const char* path = stacEnv("PATH");
		std::string paths = path && *path ? std::string(path) + ":" + gResourceDir : gResourceDir;
		setenv("STAC_PATH", paths.c_str(), 1);
		SapfEngine& engine = GetSapfEngine();
		engine.configure(config);
		engine.initialize(); // no-op after the first instance in this process
		setPostHandler([this](const char* text) { onPost(text); });

		evaluator_ = std::make_unique<Evaluator>([this](std::string* capture) {
			std::lock_guard<std::mutex> lock(logMutex_);
			capture_ = capture;
		});

		bool portFromEnv = false;
		udpPort_ = ConfiguredUdpPort(&portFromEnv);
		setUdp(portFromEnv);
		return true;
	}

	bool activate(double sampleRate)
	{
		startHostDrivenScsynth(sampleRate, kChannels);
		evaluator_->setSampleRate(sampleRate);
		return true;
	}

	static clap_process_status Process(const clap_process_t* process)
	{
		if (process->audio_outputs_count < 1) return CLAP_PROCESS_CONTINUE;
		clap_audio_buffer_t& out = process->audio_outputs[0];
		if (!out.data32) return CLAP_PROCESS_CONTINUE;
		renderHostDrivenScsynth(out.data32, (int)out.channel_count, (int)process->frames_count);
		out.constant_mask = 0;
		return CLAP_PROCESS_CONTINUE;
	}

	// --- output -------------------------------------------------------------

	void onPost(const char* text)
	{
		std::lock_guard<std::mutex> lock(logMutex_);
		if (pendingLog_.size() < kMaxPendingLog) pendingLog_ += text;
		if (capture_ && evaluator_ && std::this_thread::get_id() == evaluator_->threadId()) *capture_ += text;
	}

	void log(const std::string& text)
	{
		std::lock_guard<std::mutex> lock(logMutex_);
		if (pendingLog_.size() < kMaxPendingLog) pendingLog_ += text;
	}

	std::string drainLog()
	{
		std::lock_guard<std::mutex> lock(logMutex_);
		std::string out;
		out.swap(pendingLog_);
		return out;
	}

	// --- input --------------------------------------------------------------

	void evalFromEditor(const std::string& code)
	{
		log("stac> " + code + (code.empty() || code.back() != '\n' ? "\n" : ""));
		evaluator_->eval(code);
	}

	std::string evalFromUdp(const std::string& code)
	{
		log("udp> " + code + (code.empty() || code.back() != '\n' ? "\n" : ""));
		auto reply = std::make_shared<std::promise<std::string>>();
		std::future<std::string> result = reply->get_future();
		evaluator_->eval(code, reply);
		if (result.wait_for(kUdpReplyTimeout) != std::future_status::ready) {
			return "busy: evaluation still running\n";
		}
		try {
			std::string out = result.get();
			return out.empty() ? "ok\n" : out;
		} catch (const std::future_error&) {
			return "error: evaluator stopped\n";
		}
	}

	void setUdp(bool on)
	{
		udp_.stop();
		if (on) {
			on = udp_.start(udpPort_, [this](const std::string& msg) { return evalFromUdp(msg); });
			log(on ? "listening on UDP 127.0.0.1:" + std::to_string(udpPort_) + "\n"
			       : "could not bind UDP port " + std::to_string(udpPort_) + "\n");
		}
		udpOn_ = on;
		if (editor_) editor_->setUdpState(on);
	}

	// --- extensions ---------------------------------------------------------

	const void* extension(const char* id)
	{
		if (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &kAudioPorts;
		if (!strcmp(id, CLAP_EXT_STATE)) return &kState;
		if (!strcmp(id, CLAP_EXT_GUI)) return &kGui;
		if (!strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &kTimer;
		return nullptr;
	}

	static const clap_plugin_audio_ports_t kAudioPorts;
	static const clap_plugin_state_t kState;
	static const clap_plugin_gui_t kGui;
	static const clap_plugin_timer_support_t kTimer;

	// State: version byte, flags byte, editor text. "No wrap" is a flag so a zero byte means the defaults.
	enum : char { kStateUdp = 1, kStateNoWrap = 2 };
	bool saveState(const clap_ostream_t* stream)
	{
		std::string data;
		data += '\x01';
		data += (char)((udpOn_ ? kStateUdp : 0) | (wrap_ ? 0 : kStateNoWrap));
		data += editorText_;
		size_t written = 0;
		while (written < data.size()) {
			int64_t n = stream->write(stream, data.data() + written, data.size() - written);
			if (n <= 0) return false;
			written += (size_t)n;
		}
		return true;
	}

	bool loadState(const clap_istream_t* stream)
	{
		std::string data;
		char buf[4096];
		for (;;) {
			int64_t n = stream->read(stream, buf, sizeof(buf));
			if (n < 0) return false;
			if (n == 0) break;
			data.append(buf, (size_t)n);
		}
		if (data.size() < 2 || data[0] != '\x01') return false;
		editorText_ = data.substr(2);
		wrap_ = !(data[1] & kStateNoWrap);
		if (editor_) {
			editor_->setText(editorText_);
			editor_->setWrap(wrap_);
		}
		setUdp(data[1] & kStateUdp);
		return true;
	}

	bool createEditor()
	{
		EditorCallbacks callbacks;
		callbacks.eval = [this](const std::string& code) { evalFromEditor(code); };
		callbacks.stop = [] { stopPlaying(); };
		callbacks.setUdp = [this](bool on) { setUdp(on); };
		callbacks.setWrap = [this](bool on) { wrap_ = on; };
		callbacks.textChanged = [this](const std::string& text) { editorText_ = text; };
		callbacks.drainLog = [this] { return drainLog(); };
		editor_ = CreateEditor(std::move(callbacks), EditorState{editorText_, udpOn_, wrap_, udpPort_});
		if (!editor_) return false;
		// The host timer drives the editor; on X11 nothing else dispatches its events.
		auto* timers = (const clap_host_timer_support_t*)host_->get_extension(host_, CLAP_EXT_TIMER_SUPPORT);
		if (!timers || !timers->register_timer(host_, 16, &timerId_)) {
			timerId_ = CLAP_INVALID_ID;
			if (!strcmp(EditorWindowApi(), CLAP_WINDOW_API_X11)) log("host has no timer support; the editor will not update\n");
		}
		return true;
	}

	void destroyEditor()
	{
		if (timerId_ != CLAP_INVALID_ID) {
			auto* timers = (const clap_host_timer_support_t*)host_->get_extension(host_, CLAP_EXT_TIMER_SUPPORT);
			if (timers) timers->unregister_timer(host_, timerId_);
			timerId_ = CLAP_INVALID_ID;
		}
		editor_.reset();
	}

	clap_plugin_t plugin_{};
	const clap_host_t* host_;
	std::unique_ptr<Evaluator> evaluator_;
	UdpServer udp_;
	std::unique_ptr<Editor> editor_;
	clap_id timerId_ = CLAP_INVALID_ID;
	std::string editorText_ = "[[440 442] sinosc .1 *] play\n";
	uint16_t udpPort_ = kDefaultUdpPort;
	bool udpOn_ = false;
	bool wrap_ = true;
	unsigned width_ = kEditorWidth, height_ = kEditorHeight;

	std::mutex logMutex_;
	std::string pendingLog_;
	std::string* capture_ = nullptr;
};

const clap_plugin_audio_ports_t StacPlugin::kAudioPorts = {
	[](const clap_plugin_t*, bool isInput) -> uint32_t { return isInput ? 0 : 1; },
	[](const clap_plugin_t*, uint32_t index, bool isInput, clap_audio_port_info_t* info) {
		if (isInput || index != 0) return false;
		info->id = 0;
		snprintf(info->name, sizeof(info->name), "%s", "out");
		info->flags = CLAP_AUDIO_PORT_IS_MAIN;
		info->channel_count = kChannels;
		info->port_type = CLAP_PORT_STEREO;
		info->in_place_pair = CLAP_INVALID_ID;
		return true;
	},
};

const clap_plugin_state_t StacPlugin::kState = {
	[](const clap_plugin_t* p, const clap_ostream_t* s) { return self(p)->saveState(s); },
	[](const clap_plugin_t* p, const clap_istream_t* s) { return self(p)->loadState(s); },
};

const clap_plugin_gui_t StacPlugin::kGui = {
	// is_api_supported
	[](const clap_plugin_t*, const char* api, bool floating) {
		return !floating && !strcmp(api, EditorWindowApi());
	},
	// get_preferred_api
	[](const clap_plugin_t*, const char** api, bool* floating) {
		*api = EditorWindowApi();
		*floating = false;
		return true;
	},
	// create
	[](const clap_plugin_t* p, const char* api, bool floating) {
		return !floating && !strcmp(api, EditorWindowApi()) && self(p)->createEditor();
	},
	// destroy
	[](const clap_plugin_t* p) { self(p)->destroyEditor(); },
	// set_scale: pugl reads the platform scale factor itself
	[](const clap_plugin_t*, double) { return false; },
	// get_size
	[](const clap_plugin_t* p, uint32_t* w, uint32_t* h) {
		*w = self(p)->width_;
		*h = self(p)->height_;
		return true;
	},
	// can_resize
	[](const clap_plugin_t*) { return true; },
	// get_resize_hints
	[](const clap_plugin_t*, clap_gui_resize_hints_t* hints) {
		hints->can_resize_horizontally = true;
		hints->can_resize_vertically = true;
		hints->preserve_aspect_ratio = false;
		hints->aspect_ratio_width = 0;
		hints->aspect_ratio_height = 0;
		return true;
	},
	// adjust_size
	[](const clap_plugin_t*, uint32_t* w, uint32_t* h) {
		*w = std::max<uint32_t>(*w, 480);
		*h = std::max<uint32_t>(*h, 320);
		return true;
	},
	// set_size
	[](const clap_plugin_t* p, uint32_t w, uint32_t h) {
		StacPlugin* s = self(p);
		s->width_ = w;
		s->height_ = h;
		if (s->editor_) s->editor_->setSize(w, h);
		return true;
	},
	// set_parent
	[](const clap_plugin_t* p, const clap_window_t* window) {
		return self(p)->editor_ && self(p)->editor_->setParent(window);
	},
	// set_transient
	[](const clap_plugin_t*, const clap_window_t*) { return false; },
	// suggest_title
	[](const clap_plugin_t*, const char*) {},
	// show
	[](const clap_plugin_t* p) {
		if (!self(p)->editor_) return false;
		self(p)->editor_->show();
		return true;
	},
	// hide
	[](const clap_plugin_t* p) {
		if (!self(p)->editor_) return false;
		self(p)->editor_->hide();
		return true;
	},
};

const clap_plugin_timer_support_t StacPlugin::kTimer = {
	[](const clap_plugin_t* p, clap_id id) {
		StacPlugin* s = self(p);
		if (s->editor_ && id == s->timerId_) s->editor_->idle();
	},
};

// --- factory and entry ------------------------------------------------------

const clap_plugin_factory_t kFactory = {
	[](const clap_plugin_factory_t*) -> uint32_t { return 1; },
	[](const clap_plugin_factory_t*, uint32_t index) -> const clap_plugin_descriptor_t* {
		return index == 0 ? &gDescriptor : nullptr;
	},
	[](const clap_plugin_factory_t*, const clap_host_t* host, const char* id) -> const clap_plugin_t* {
		if (!clap_version_is_compatible(host->clap_version) || strcmp(id, gDescriptor.id)) return nullptr;
		if (gInstanceExists.exchange(true)) {
			fprintf(stderr, "stac: only one instance per process is supported\n");
			return nullptr;
		}
		return (new StacPlugin(host))->clapPlugin();
	},
};

} // namespace

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
	CLAP_VERSION_INIT,
	[](const char* pluginPath) {
		gDescriptor.version = SapfGetVersionString();
		gDescriptor.features = kFeatures;
		gResourceDir = ResourceDir(pluginPath);
		return true;
	},
	[]() {},
	[](const char* factoryId) -> const void* {
		return strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &kFactory;
	},
};
