// Minimal CLAP host: loads the stac plugin and checks UDP evaluation, audio output and its timing,
// state round trip, and the one-instance rule. Usage: test_clap_host <plugin binary>
// Expects STAC_CLAP_UDP_PORT to be set, which turns UDP on at init.
//
// An audio thread calls process() at about real time throughout, as a host does: play waits for
// scsynth's reply, and the host-driven scsynth handles commands only inside process().

#include <clap/clap.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <dlfcn.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace {

const uint32_t kFrames = 100; // not a multiple of scsynth's 64-frame block

int gFailures = 0;

void check(bool ok, const char* what)
{
	printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
	if (!ok) ++gFailures;
}

const clap_plugin_entry_t* loadEntry(const char* path)
{
	void* lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!lib) fprintf(stderr, "dlopen: %s\n", dlerror());
	return lib ? (const clap_plugin_entry_t*)dlsym(lib, "clap_entry") : nullptr;
}

const clap_host_t kHost = {
	CLAP_VERSION_INIT,
	nullptr,
	"test_clap_host",
	"stac",
	"",
	"1.0",
	[](const clap_host_t*, const char*) -> const void* { return nullptr; },
	[](const clap_host_t*) {},
	[](const clap_host_t*) {},
	[](const clap_host_t*) {},
};

// Sends one datagram and returns the reply ("" on timeout).
std::string udpEval(int port, const std::string& code)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	timeval timeout{15, 0};
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sendto(fd, code.data(), (int)code.size(), 0, (sockaddr*)&addr, sizeof(addr));
	char buf[65536];
	int n = (int)recv(fd, buf, sizeof(buf), 0);
	close(fd);
	return n > 0 ? std::string(buf, n) : std::string();
}

void sleepSeconds(double s) { std::this_thread::sleep_for(std::chrono::duration<double>(s)); }

// Calls process() every kFrames / speed of real time, tracking the peak and the first loud frame.
class AudioThread {
public:
	AudioThread(const clap_plugin_t* plugin, double rate, double speed = 1.) : plugin_(plugin), rate_(rate), speed_(speed)
	{
		thread_ = std::thread([this] { run(); });
	}

	~AudioThread()
	{
		quit_ = true;
		thread_.join();
	}

	double rate() const { return rate_; }
	int64_t frames() const { return frames_; }
	float peak() const { return peak_; }
	int64_t firstLoud() const { return firstLoud_; }
	void resetPeak()
	{
		peak_ = 0.f;
		firstLoud_ = -1;
	}

private:
	void run()
	{
		std::vector<float> left(kFrames), right(kFrames);
		float* channels[2] = {left.data(), right.data()};
		clap_audio_buffer_t out{};
		out.data32 = channels;
		out.channel_count = 2;
		clap_process_t process{};
		process.frames_count = kFrames;
		process.audio_outputs = &out;
		process.audio_outputs_count = 1;
		plugin_->start_processing(plugin_);
		auto due = std::chrono::steady_clock::now();
		while (!quit_) {
			plugin_->process(plugin_, &process);
			float p = 0.f;
			for (uint32_t i = 0; i < kFrames; ++i) p = std::max({p, std::fabs(left[i]), std::fabs(right[i])});
			if (p > peak_) peak_ = p;
			if (p > 0.01f && firstLoud_ < 0) firstLoud_ = frames_.load();
			frames_ += kFrames;
			due += std::chrono::microseconds((int64_t)(kFrames * 1e6 / (rate_ * speed_)));
			std::this_thread::sleep_until(due);
		}
		plugin_->stop_processing(plugin_);
	}

	const clap_plugin_t* plugin_;
	double rate_, speed_;
	std::thread thread_;
	std::atomic<bool> quit_{false};
	std::atomic<int64_t> frames_{0}, firstLoud_{-1};
	std::atomic<float> peak_{0.f};
};

struct Buffer {
	std::string data;
	size_t pos = 0;
};

int64_t writeBuffer(const clap_ostream_t* s, const void* p, uint64_t n)
{
	((Buffer*)s->ctx)->data.append((const char*)p, n);
	return (int64_t)n;
}

int64_t readBuffer(const clap_istream_t* s, void* p, uint64_t n)
{
	Buffer* b = (Buffer*)s->ctx;
	size_t k = std::min<size_t>(n, b->data.size() - b->pos);
	memcpy(p, b->data.data() + b->pos, k);
	b->pos += k;
	return (int64_t)k;
}

// Stops what plays and checks for silence.
void checkStop(int port, AudioThread& audio, const std::string& label)
{
	check(udpEval(port, "stop") == "ok\n", (label + ": udp stop").c_str());
	sleepSeconds(.05);
	audio.resetPeak();
	sleepSeconds(.2);
	check(audio.peak() == 0.f, (label + ": stop silences output").c_str());
}

// Plays a sine at .1, with sc's sinosc since the sapf module replaces sinosc, then a pattern,
// whose events are bundles timed .2 s ahead of the plugin's clock, which counts rendered frames.
void checkPlay(int port, AudioThread& audio, const std::string& label)
{
	audio.resetPeak();
	std::string reply = udpEval(port, "[440 sc.sinosc .1 *] play clear");
	check(reply == "ok\n", (label + ": udp play").c_str());
	if (reply != "ok\n") printf("reply: %s\n", reply.c_str());
	sleepSeconds(.3);
	printf("peak %.4f\n", audio.peak());
	check(audio.peak() > 0.09f && audio.peak() < 0.11f, (label + ": plays sine at 0.1").c_str());
	checkStop(port, audio, label);

	udpEval(port, "synthdef ping freq:440 amp:.1 gate:1 [freq sc.sinosc .005 .2 env.asr gate envgen.kr.done * amp *]");
	audio.resetPeak();
	reply = udpEval(port, "{:instrument 'ping :midinote [69] cyc :dur .25} pbind play clear");
	const int64_t replied = audio.frames();
	check(reply == "ok\n", (label + ": udp pattern").c_str());
	if (reply != "ok\n") printf("reply: %s\n", reply.c_str());
	sleepSeconds(.5);
	const double delay = audio.firstLoud() < 0 ? -1. : (audio.firstLoud() - replied) / audio.rate();
	printf("first event %.3f s after the reply\n", delay);
	check(delay > .1 && delay < .25, (label + ": first event .2 s after play").c_str());
	checkStop(port, audio, label);
}

} // namespace

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: test_clap_host <plugin>\n");
		return 2;
	}
	const char* portEnv = getenv("STAC_CLAP_UDP_PORT");
	if (!portEnv) {
		fprintf(stderr, "STAC_CLAP_UDP_PORT must be set\n");
		return 2;
	}
	int port = atoi(portEnv);

	const clap_plugin_entry_t* entry = loadEntry(argv[1]);
	check(entry != nullptr, "clap_entry exported");
	if (!entry) return 1;
	check(entry->init(argv[1]), "entry init");
	auto* factory = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
	check(factory && factory->get_plugin_count(factory) == 1, "factory has one plugin");
	const char* id = factory->get_plugin_descriptor(factory, 0)->id;

	const clap_plugin_t* plugin = factory->create_plugin(factory, &kHost, id);
	check(plugin != nullptr, "create plugin");
	check(factory->create_plugin(factory, &kHost, id) == nullptr, "second instance refused");
	check(plugin->init(plugin), "plugin init");
	check(udpEval(port, "[440 sinosc] play").find("not running audio") != std::string::npos,
	      "play before activate fails");
	udpEval(port, "clear");
	check(plugin->activate(plugin, 48000., kFrames, kFrames), "activate");
	{
		AudioThread audio(plugin, 48000.);
		check(udpEval(port, "1 2 +").find('3') != std::string::npos, "udp eval replies with stack");
		check(udpEval(port, "nosuchword").find("error") != std::string::npos, "udp eval replies with error");
		check(udpEval(port, "clear") == "ok\n", "udp clear leaves an empty stack");
		check(udpEval(port, "sr").find("48000") != std::string::npos, "sr is the host's rate");
		check(udpEval(port, "440 sinosc clear") == "ok\n", "bundled prelude imports sc");
		checkPlay(port, audio, "first instance");
		// last: sapf's words replace sc's, such as sinosc
		std::string sapf = udpEval(port, "import sapf.*\nclear");
		check(sapf.find("error") == std::string::npos, "bundled sapf module imports");
		if (sapf.find("error") != std::string::npos) printf("reply: %s\n", sapf.c_str());
	}

	auto* state = (const clap_plugin_state_t*)plugin->get_extension(plugin, CLAP_EXT_STATE);
	Buffer saved;
	clap_ostream_t os{&saved, writeBuffer};
	check(state && state->save(plugin, &os) && saved.data.size() >= 2, "state save");
	clap_istream_t is{&saved, readBuffer};
	check(state->load(plugin, &is), "state load");
	check((saved.data[1] & 2) == 0, "wrap is on by default");
	Buffer noWrap;
	noWrap.data = std::string("\x01\x03", 2) + "1 2 +"; // UDP on, wrap off
	clap_istream_t is2{&noWrap, readBuffer};
	Buffer resaved;
	clap_ostream_t os2{&resaved, writeBuffer};
	check(state->load(plugin, &is2) && state->save(plugin, &os2) && resaved.data == noWrap.data,
	      "state round-trips wrap off and editor text");

	plugin->deactivate(plugin);
	std::string late = udpEval(port, "[440 sc.sinosc] play");
	check(late.find("not running audio") != std::string::npos, "play after deactivate fails");
	if (late.find("not running audio") == std::string::npos) printf("reply: %s\n", late.c_str());
	plugin->destroy(plugin);

	// A new instance boots a new World, at another rate. Rendering at twice real time shows that
	// bundle times follow the rendered frames, not the wall clock.
	const clap_plugin_t* again = factory->create_plugin(factory, &kHost, id);
	check(again != nullptr, "new instance after destroy");
	if (again && again->init(again) && again->activate(again, 44100., kFrames, kFrames)) {
		{
			AudioThread audio(again, 44100., 2.);
			check(udpEval(port, "sr").find("44100") != std::string::npos, "second instance: sr is the new rate");
			checkPlay(port, audio, "second instance");
		}
		again->deactivate(again);
	}
	if (again) again->destroy(again);

	entry->deinit();
	printf("%d failure(s)\n", gFailures);
	return gFailures ? 1 : 0;
}
