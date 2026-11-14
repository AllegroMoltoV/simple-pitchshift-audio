#include "plugin-entry.h"

#include <clap/clap.h>
#include <clapwrapper/vst3.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <xmmintrin.h>

#include "voice-pitch-shifter.h"

#ifndef SIMPLE_PITCHSHIFT_VERSION
#define SIMPLE_PITCHSHIFT_VERSION "0.0.0"
#endif

namespace {

enum ParamId : clap_id { kPitch = 0, kFormant = 1, kGain = 2, kWindow = 3, kParamCount = 4 };

struct ParamInfo {
	clap_id id;
	const char *name;
	double min;
	double max;
	double defaultValue;
	clap_param_info_flags flags;
};

constexpr std::array<ParamInfo, kParamCount> kParams{{
	{kPitch, "Pitch", -12.0, 12.0, 0.0, CLAP_PARAM_IS_AUTOMATABLE},
	{kFormant, "Formant", -12.0, 12.0, 0.0, CLAP_PARAM_IS_AUTOMATABLE},
	{kGain, "Gain", -30.0, 30.0, 0.0, CLAP_PARAM_IS_AUTOMATABLE},
	{kWindow, "Window", 0.0, 2.0, double(VoicePitchShifter::kDefaultWindowIndex),
	 CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM},
}};

const char *const kFeatures[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_PITCH_SHIFTER,
				 CLAP_PLUGIN_FEATURE_STEREO, nullptr};

const clap_plugin_descriptor kDescriptor = {
	CLAP_VERSION_INIT,
	"jp.allegromoltov.simple-pitchshift",
	"Simple Pitch Shift",
	"AllegroMoltoV",
	"https://github.com/AllegroMoltoV/simple-pitchshift-audio",
	"https://github.com/AllegroMoltoV/simple-pitchshift-audio",
	"https://github.com/AllegroMoltoV/simple-pitchshift-audio",
	SIMPLE_PITCHSHIFT_VERSION,
	"Changes voice pitch and formant in real time.",
	kFeatures,
};

constexpr uint32_t kStateMagic = 0x53505331; // "SPS1"

struct Plugin {
	clap_plugin plugin{};
	const clap_host *host = nullptr;
	// Values shared between the main thread and the audio thread.
	std::array<std::atomic<double>, kParamCount> values{};
	// Audio-thread state.
	VoicePitchShifter shifter;
	float appliedGain = 1.0f;
	bool active = false;
};

Plugin *self(const clap_plugin *plugin)
{
	return static_cast<Plugin *>(plugin->plugin_data);
}

double clampParam(clap_id id, double value)
{
	const auto &info = kParams[id];
	value = std::clamp(value, info.min, info.max);
	return (info.flags & CLAP_PARAM_IS_STEPPED) ? std::round(value) : value;
}

float gainFactor(double db)
{
	return float(std::pow(10.0, db / 20.0));
}

void applyEvents(Plugin *p, const clap_input_events *events)
{
	const uint32_t count = events->size(events);
	for (uint32_t i = 0; i < count; ++i) {
		const clap_event_header *header = events->get(events, i);
		if (header->space_id != CLAP_CORE_EVENT_SPACE_ID || header->type != CLAP_EVENT_PARAM_VALUE)
			continue;
		const auto *event = reinterpret_cast<const clap_event_param_value *>(header);
		if (event->param_id < kParamCount)
			p->values[event->param_id].store(clampParam(event->param_id, event->value),
							 std::memory_order_relaxed);
	}
}

// Ramps from the previous block's gain to the current one so parameter moves do not click.
void applyGain(Plugin *p, float *const *channels, uint32_t channelCount, uint32_t frames)
{
	const float target = gainFactor(p->values[kGain].load(std::memory_order_relaxed));
	const float start = p->appliedGain;
	p->appliedGain = target;
	if (start == 1.0f && target == 1.0f)
		return;
	for (uint32_t c = 0; c < channelCount; ++c)
		for (uint32_t i = 0; i < frames; ++i)
			channels[c][i] *= start + (target - start) * float(i + 1) / float(frames);
}

// Flushes denormals to zero while processing, because the pitch shifter's tails decay into them and they
// slow the processing down. The previous mode is restored so the host's own settings are not changed.
class FlushDenormals {
public:
	FlushDenormals() : saved_(_mm_getcsr())
	{
		_mm_setcsr(saved_ | 0x8040); // FTZ (bit 15) and DAZ (bit 6)
	}
	~FlushDenormals()
	{
		_mm_setcsr(saved_);
	}
	FlushDenormals(const FlushDenormals &) = delete;
	FlushDenormals &operator=(const FlushDenormals &) = delete;

private:
	unsigned int saved_;
};

// --- clap_plugin ---

bool pluginInit(const clap_plugin *plugin)
{
	auto *p = self(plugin);
	for (const auto &info : kParams)
		p->values[info.id].store(info.defaultValue, std::memory_order_relaxed);
	return true;
}

void pluginDestroy(const clap_plugin *plugin)
{
	delete self(plugin);
}

bool pluginActivate(const clap_plugin *plugin, double sampleRate, uint32_t, uint32_t)
{
	auto *p = self(plugin);
	p->shifter.prepare(sampleRate);
	p->appliedGain = gainFactor(p->values[kGain].load(std::memory_order_relaxed));
	p->active = true;
	return true;
}

void pluginDeactivate(const clap_plugin *plugin)
{
	self(plugin)->active = false;
}

bool pluginStartProcessing(const clap_plugin *)
{
	return true;
}

void pluginStopProcessing(const clap_plugin *) {}

void pluginReset(const clap_plugin *plugin)
{
	self(plugin)->shifter.reset();
}

clap_process_status pluginProcess(const clap_plugin *plugin, const clap_process *process)
{
	auto *p = self(plugin);
	const FlushDenormals flush;
	applyEvents(p, process->in_events);
	const uint32_t frames = process->frames_count;
	if (process->audio_inputs_count < 1 || process->audio_outputs_count < 1 || frames == 0)
		return CLAP_PROCESS_CONTINUE;
	const clap_audio_buffer &in = process->audio_inputs[0];
	const clap_audio_buffer &out = process->audio_outputs[0];
	if (!in.data32 || !out.data32)
		return CLAP_PROCESS_ERROR;
	const uint32_t channels = std::min(in.channel_count, out.channel_count);
	for (uint32_t c = 0; c < channels; ++c)
		if (out.data32[c] != in.data32[c])
			std::memcpy(out.data32[c], in.data32[c], sizeof(float) * frames);
	if (channels == 2)
		p->shifter.process(out.data32[0], out.data32[1], frames,
				   VoicePitchShifter::clampWindowIndex(int(p->values[kWindow].load(std::memory_order_relaxed))),
				   float(p->values[kPitch].load(std::memory_order_relaxed)),
				   float(p->values[kFormant].load(std::memory_order_relaxed)));
	applyGain(p, out.data32, channels, frames);
	return CLAP_PROCESS_CONTINUE;
}

// --- audio ports ---

uint32_t portsCount(const clap_plugin *, bool)
{
	return 1;
}

bool portsGet(const clap_plugin *, uint32_t index, bool isInput, clap_audio_port_info *info)
{
	if (index != 0)
		return false;
	info->id = 0;
	std::snprintf(info->name, sizeof(info->name), "%s", isInput ? "Input" : "Output");
	info->flags = CLAP_AUDIO_PORT_IS_MAIN;
	info->channel_count = 2;
	info->port_type = CLAP_PORT_STEREO;
	info->in_place_pair = 0;
	return true;
}

const clap_plugin_audio_ports kAudioPorts = {portsCount, portsGet};

// --- params ---

uint32_t paramsCount(const clap_plugin *)
{
	return kParamCount;
}

bool paramsGetInfo(const clap_plugin *, uint32_t index, clap_param_info *info)
{
	if (index >= kParamCount)
		return false;
	const auto &param = kParams[index];
	std::memset(info, 0, sizeof(*info));
	info->id = param.id;
	info->flags = param.flags;
	info->min_value = param.min;
	info->max_value = param.max;
	info->default_value = param.defaultValue;
	std::snprintf(info->name, sizeof(info->name), "%s", param.name);
	return true;
}

bool paramsGetValue(const clap_plugin *plugin, clap_id id, double *value)
{
	if (id >= kParamCount)
		return false;
	*value = self(plugin)->values[id].load(std::memory_order_relaxed);
	return true;
}

bool paramsValueToText(const clap_plugin *, clap_id id, double value, char *text, uint32_t size)
{
	if (id >= kParamCount)
		return false;
	switch (id) {
	case kPitch:
	case kFormant:
		std::snprintf(text, size, "%+.1f st", value);
		return true;
	case kGain:
		std::snprintf(text, size, "%+.1f dB", value);
		return true;
	case kWindow:
		std::snprintf(text, size, "%d ms",
			      VoicePitchShifter::kWindowMs[VoicePitchShifter::clampWindowIndex(int(std::round(value)))]);
		return true;
	default:
		return false;
	}
}

bool paramsTextToValue(const clap_plugin *, clap_id id, const char *text, double *value)
{
	if (id >= kParamCount)
		return false;
	double parsed = 0.0;
	if (std::sscanf(text, "%lf", &parsed) != 1)
		return false;
	if (id == kWindow) {
		for (size_t w = 0; w < VoicePitchShifter::kWindowMs.size(); ++w)
			if (int(std::round(parsed)) == VoicePitchShifter::kWindowMs[w]) {
				*value = double(w);
				return true;
			}
		return false;
	}
	*value = clampParam(id, parsed);
	return true;
}

void paramsFlush(const clap_plugin *plugin, const clap_input_events *in, const clap_output_events *)
{
	applyEvents(self(plugin), in);
}

const clap_plugin_params kParamsExt = {paramsCount, paramsGetInfo, paramsGetValue, paramsValueToText,
				       paramsTextToValue, paramsFlush};

// --- latency ---

uint32_t latencyGet(const clap_plugin *plugin)
{
	return self(plugin)->shifter.latency();
}

const clap_plugin_latency kLatency = {latencyGet};

// --- state ---

bool writeAll(const clap_ostream *stream, const void *data, size_t size)
{
	const auto *bytes = static_cast<const uint8_t *>(data);
	while (size > 0) {
		const int64_t written = stream->write(stream, bytes, size);
		if (written <= 0)
			return false;
		bytes += written;
		size -= size_t(written);
	}
	return true;
}

bool readAll(const clap_istream *stream, void *data, size_t size)
{
	auto *bytes = static_cast<uint8_t *>(data);
	while (size > 0) {
		const int64_t read = stream->read(stream, bytes, size);
		if (read <= 0)
			return false;
		bytes += read;
		size -= size_t(read);
	}
	return true;
}

bool stateSave(const clap_plugin *plugin, const clap_ostream *stream)
{
	auto *p = self(plugin);
	const uint32_t header[2] = {kStateMagic, kParamCount};
	if (!writeAll(stream, header, sizeof(header)))
		return false;
	for (const auto &value : p->values) {
		const double v = value.load(std::memory_order_relaxed);
		if (!writeAll(stream, &v, sizeof(v)))
			return false;
	}
	return true;
}

bool stateLoad(const clap_plugin *plugin, const clap_istream *stream)
{
	auto *p = self(plugin);
	uint32_t header[2] = {};
	if (!readAll(stream, header, sizeof(header)) || header[0] != kStateMagic)
		return false;
	for (uint32_t i = 0; i < header[1]; ++i) {
		double v = 0.0;
		if (!readAll(stream, &v, sizeof(v)))
			return false;
		if (i < kParamCount)
			p->values[i].store(clampParam(i, v), std::memory_order_relaxed);
	}
	// The loaded values replace what the host shows, so the host has to read them again.
	const auto *hostParams = static_cast<const clap_host_params *>(p->host->get_extension(p->host, CLAP_EXT_PARAMS));
	if (hostParams && hostParams->rescan)
		hostParams->rescan(p->host, CLAP_PARAM_RESCAN_VALUES);
	return true;
}

const clap_plugin_state kState = {stateSave, stateLoad};

const void *pluginGetExtension(const clap_plugin *, const char *id)
{
	if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS))
		return &kAudioPorts;
	if (!std::strcmp(id, CLAP_EXT_PARAMS))
		return &kParamsExt;
	if (!std::strcmp(id, CLAP_EXT_LATENCY))
		return &kLatency;
	if (!std::strcmp(id, CLAP_EXT_STATE))
		return &kState;
	return nullptr;
}

void pluginOnMainThread(const clap_plugin *) {}

const clap_plugin *createPlugin(const clap_host *host)
{
	auto *p = new Plugin();
	p->host = host;
	p->plugin.desc = &kDescriptor;
	p->plugin.plugin_data = p;
	p->plugin.init = pluginInit;
	p->plugin.destroy = pluginDestroy;
	p->plugin.activate = pluginActivate;
	p->plugin.deactivate = pluginDeactivate;
	p->plugin.start_processing = pluginStartProcessing;
	p->plugin.stop_processing = pluginStopProcessing;
	p->plugin.reset = pluginReset;
	p->plugin.process = pluginProcess;
	p->plugin.get_extension = pluginGetExtension;
	p->plugin.on_main_thread = pluginOnMainThread;
	return &p->plugin;
}

// --- factory ---

uint32_t factoryCount(const clap_plugin_factory *)
{
	return 1;
}

const clap_plugin_descriptor *factoryDescriptor(const clap_plugin_factory *, uint32_t index)
{
	return index == 0 ? &kDescriptor : nullptr;
}

const clap_plugin *factoryCreate(const clap_plugin_factory *, const clap_host *host, const char *pluginId)
{
	if (!clap_version_is_compatible(host->clap_version) || std::strcmp(pluginId, kDescriptor.id) != 0)
		return nullptr;
	return createPlugin(host);
}

const clap_plugin_factory kFactory = {factoryCount, factoryDescriptor, factoryCreate};

// Without this, clap-wrapper fills the VST 3 factory's e-mail contact with the placeholder "info@".
// The other members stay nullptr, so the VST 3 class ID is the one clap-wrapper derives from the CLAP ID.
const clap_plugin_factory_as_vst3 kVst3FactoryInfo = {nullptr, kDescriptor.url, "", nullptr, nullptr};

} // namespace

bool sps_entry_init(const char *)
{
	return true;
}

void sps_entry_deinit(void) {}

const void *sps_entry_get_factory(const char *factoryId)
{
	if (std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0)
		return &kFactory;
	if (std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_INFO_VST3) == 0 ||
	    std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_INFO_VST3_V1) == 0)
		return &kVst3FactoryInfo;
	return nullptr;
}
