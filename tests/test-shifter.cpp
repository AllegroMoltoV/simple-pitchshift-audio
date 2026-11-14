// Offline checks of VoicePitchShifter. Exits with a non-zero status on the first failure.
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

// Runs the signal through the shifter in host-sized blocks.
std::vector<float> run(VoicePitchShifter &shifter, std::vector<float> signal, size_t window, float semitones,
		       float formant, uint32_t block = 512)
{
	std::vector<float> right = signal;
	for (size_t offset = 0; offset < signal.size(); offset += block) {
		const auto count = uint32_t(std::min<size_t>(block, signal.size() - offset));
		shifter.process(signal.data() + offset, right.data() + offset, count, window, semitones, formant);
	}
	return signal;
}

// Counts upward zero crossings in the second half to estimate the frequency of a steady tone.
double estimateFrequency(const std::vector<float> &signal, double rate)
{
	const size_t start = signal.size() / 2;
	size_t crossings = 0;
	for (size_t i = start + 1; i < signal.size(); ++i)
		if (signal[i - 1] < 0.0f && signal[i] >= 0.0f)
			++crossings;
	return double(crossings) * rate / double(signal.size() - start);
}

void testRate(double rate)
{
	VoicePitchShifter shifter;
	shifter.prepare(rate);
	const uint32_t latency = shifter.latency();
	check(latency > 0 && latency < uint32_t(rate * 0.2), "latency is positive and below 200 ms", rate);

	const size_t length = size_t(rate * 1.5);

	// Bypass delays an impulse by exactly the reported latency.
	std::vector<float> impulse(length, 0.0f);
	impulse[100] = 1.0f;
	shifter.reset();
	auto out = run(shifter, impulse, VoicePitchShifter::kDefaultWindowIndex, 0.0f, 0.0f);
	check(std::fabs(out[100 + latency] - 1.0f) < 1e-6f, "bypass delays by the reported latency", rate);

	// Silence stays silent while shifting, for every window.
	for (size_t w = 0; w < VoicePitchShifter::kWindowMs.size(); ++w) {
		shifter.reset();
		auto silent = run(shifter, std::vector<float>(length, 0.0f), w, 5.0f, 0.0f);
		float peak = 0.0f;
		for (float s : silent)
			peak = std::max(peak, std::fabs(s));
		check(peak < 1e-6f, "silence stays silent", rate);
	}

	// A tone shifted by +12 and -12 semitones doubles and halves in frequency, for every window.
	std::vector<float> tone(length);
	for (size_t i = 0; i < length; ++i)
		tone[i] = float(0.5 * std::sin(2.0 * kPi * 440.0 * double(i) / rate));
	for (size_t w = 0; w < VoicePitchShifter::kWindowMs.size(); ++w) {
		shifter.reset();
		const double up = estimateFrequency(run(shifter, tone, w, 12.0f, 0.0f), rate);
		check(std::fabs(up / 880.0 - 1.0) < 0.05, "+12 semitones doubles the frequency", rate);
		shifter.reset();
		const double down = estimateFrequency(run(shifter, tone, w, -12.0f, 0.0f), rate);
		check(std::fabs(down / 220.0 - 1.0) < 0.05, "-12 semitones halves the frequency", rate);
	}

	// The shifted tone arrives at the same time as the bypassed one, so the latency stays constant.
	std::vector<float> burst(length, 0.0f);
	const size_t burstStart = size_t(rate * 0.5);
	for (size_t i = burstStart; i < length; ++i)
		burst[i] = float(0.5 * std::sin(2.0 * kPi * 440.0 * double(i) / rate));
	for (size_t w = 0; w < VoicePitchShifter::kWindowMs.size(); ++w) {
		shifter.reset();
		auto shifted = run(shifter, burst, w, 0.1f, 0.0f);
		size_t onset = 0;
		while (onset < shifted.size() && std::fabs(shifted[onset]) < 0.05f)
			++onset;
		const double error = double(onset) - double(burstStart + latency);
		check(std::fabs(error) < rate * 0.01, "shifted onset matches the reported latency within 10 ms", rate);
	}
}

} // namespace

int main()
{
	for (double rate : {44100.0, 48000.0, 96000.0})
		testRate(rate);
	std::printf("%d failure(s)\n", failures);
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
