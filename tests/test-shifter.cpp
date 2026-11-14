// Offline checks of the behaviour that hurts users silently when it breaks.
// The DAW's delay compensation relies on a constant, correctly reported latency, so most checks guard that.
#include "voice-pitch-shifter.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;
int failures = 0;

void check(bool ok, const char *what, double rate)
{
	std::printf("%s %s at %.0f Hz\n", ok ? "ok  " : "FAIL", what, rate);
	if (!ok)
		++failures;
}

// Runs a mono signal through both channels in host-sized blocks and returns the left channel.
std::vector<float> run(VoicePitchShifter &shifter, std::vector<float> left, size_t window, float semitones,
		       float formant)
{
	constexpr uint32_t kBlock = 512;
	std::vector<float> right = left;
	for (size_t offset = 0; offset < left.size(); offset += kBlock) {
		const auto count = uint32_t(std::min<size_t>(kBlock, left.size() - offset));
		shifter.process(left.data() + offset, right.data() + offset, count, window, semitones, formant);
	}
	return left;
}

std::vector<float> sine(double rate, size_t length, size_t start)
{
	std::vector<float> out(length, 0.0f);
	for (size_t i = start; i < length; ++i)
		out[i] = float(0.5 * std::sin(2.0 * kPi * 440.0 * double(i) / rate));
	return out;
}

void latencyIsConstant(double rate)
{
	VoicePitchShifter shifter;
	shifter.prepare(rate);
	const uint32_t latency = shifter.latency();
	const size_t length = size_t(rate);

	// S1: bypass delays an impulse by exactly the reported latency.
	std::vector<float> impulse(length, 0.0f);
	impulse[100] = 1.0f;
	shifter.reset();
	const auto bypassed = run(shifter, impulse, VoicePitchShifter::kDefaultWindowIndex, 0.0f, 0.0f);
	check(std::fabs(bypassed[100 + latency] - 1.0f) < 1e-6f, "S1 bypass delays by the reported latency", rate);

	// S2: every window delivers a tone at the reported latency, so switching windows keeps alignment.
	const size_t burstStart = size_t(rate * 0.3);
	const auto burst = sine(rate, length, burstStart);
	for (size_t w = 0; w < VoicePitchShifter::kWindowMs.size(); ++w) {
		shifter.reset();
		const auto shifted = run(shifter, burst, w, 0.1f, 0.0f);
		size_t onset = 0;
		while (onset < shifted.size() && std::fabs(shifted[onset]) < 0.05f)
			++onset;
		const double error = double(onset) - double(burstStart + latency);
		check(std::fabs(error) < rate * 0.001, "S2 onset matches the reported latency within 1 ms", rate);
	}
}

void silenceStaysSilent(double rate)
{
	// S3
	VoicePitchShifter shifter;
	shifter.prepare(rate);
	const auto out = run(shifter, std::vector<float>(size_t(rate), 0.0f), VoicePitchShifter::kDefaultWindowIndex,
			     5.0f, 0.0f);
	float peak = 0.0f;
	for (float s : out)
		peak = std::max(peak, std::fabs(s));
	check(peak < 1e-6f, "S3 silence stays silent", rate);
}

void pitchGoesUp(double rate)
{
	// S4: +12 semitones doubles a 440 Hz tone, estimated from upward zero crossings in the second half.
	VoicePitchShifter shifter;
	shifter.prepare(rate);
	const auto out =
		run(shifter, sine(rate, size_t(rate * 1.5), 0), VoicePitchShifter::kDefaultWindowIndex, 12.0f, 0.0f);
	const size_t start = out.size() / 2;
	size_t crossings = 0;
	for (size_t i = start + 1; i < out.size(); ++i)
		if (out[i - 1] < 0.0f && out[i] >= 0.0f)
			++crossings;
	const double frequency = double(crossings) * rate / double(out.size() - start);
	check(std::fabs(frequency / 880.0 - 1.0) < 0.05, "S4 +12 semitones doubles the frequency", rate);
}

} // namespace

int main()
{
	for (double rate : {48000.0, 96000.0}) {
		latencyIsConstant(rate);
		silenceStaysSilent(rate);
	}
	pitchGoesUp(48000.0);
	std::printf("%d failure(s)\n", failures);
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
