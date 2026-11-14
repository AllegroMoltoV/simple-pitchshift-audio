#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <signalsmith-stretch/signalsmith-stretch.h>

// Stereo pitch and formant shifter with a constant latency.
// DAWs compensate a plugin's latency only when it stays fixed while the plugin is active,
// so every window and the bypass path are padded to the latency of the longest window.
class VoicePitchShifter {
public:
	static constexpr std::array<int, 3> kWindowMs{20, 30, 40};
	static constexpr size_t kDefaultWindowIndex = 2;
	static constexpr float kMaxSemitones = 12.0f;

	// Allocates; call outside the audio thread (CLAP activate).
	void prepare(double sampleRate)
	{
		uint32_t longest = 0;
		for (size_t w = 0; w < kWindowMs.size(); ++w) {
			const int block = int(sampleRate * kWindowMs[w] / 1000.0);
			stretches[w].configure(2, block, block / 4, false);
			stretches[w].setFormantFactor(1, true);
			stretches[w].setFormantBase(float(180.0 / sampleRate));
			windowLatency[w] = uint32_t(stretches[w].inputLatency() + stretches[w].outputLatency());
			longest = std::max(longest, windowLatency[w]);
		}
		totalLatency = longest;
		for (auto &line : delayLine)
			line.assign(totalLatency + 1, 0.0f);
		reset();
	}

	uint32_t latency() const { return totalLatency; }

	void reset()
	{
		for (auto &line : delayLine)
			std::fill(line.begin(), line.end(), 0.0f);
		writePos = 0;
		resetPending = true;
	}

	static size_t clampWindowIndex(int index) { return size_t(std::clamp(index, 0, int(kWindowMs.size()) - 1)); }

	void process(float *left, float *right, uint32_t frames, size_t window, float semitones, float formantSemitones)
	{
		semitones = std::clamp(semitones, -kMaxSemitones, kMaxSemitones);
		formantSemitones = std::clamp(formantSemitones, -kMaxSemitones, kMaxSemitones);
		window = std::min(window, kWindowMs.size() - 1);
		const bool bypass = semitones == 0.0f && formantSemitones == 0.0f;
		if (bypass) {
			resetPending = true;
			delay(left, right, frames, totalLatency);
			return;
		}
		auto &stretch = stretches[window];
		// Only a change of window or a return from bypass clears audio; pitch and formant follow without gaps.
		if (resetPending || window != previousWindow) {
			stretch.reset();
			previousWindow = window;
			resetPending = false;
			previousSemitones = semitones + 1.0f;
			previousFormant = formantSemitones + 1.0f;
		}
		if (semitones != previousSemitones) {
			stretch.setTransposeSemitones(semitones);
			previousSemitones = semitones;
		}
		if (formantSemitones != previousFormant) {
			stretch.setFormantSemitones(formantSemitones, true);
			previousFormant = formantSemitones;
		}
		// Scratch capacity is a processing chunk, not a limit on the host block size.
		for (uint32_t offset = 0; offset < frames;) {
			const auto count = std::min<uint32_t>(frames - offset, uint32_t(scratch[0].size()));
			const float *input[] = {left + offset, right + offset};
			float *output[] = {scratch[0].data(), scratch[1].data()};
			stretch.process(input, int(count), output, int(count));
			std::copy_n(output[0], count, left + offset);
			std::copy_n(output[1], count, right + offset);
			offset += count;
		}
		delay(left, right, frames, totalLatency - windowLatency[window]);
	}

private:
	using Stretch = signalsmith::stretch::SignalsmithStretch<float>;

	// Delays both channels in place by `samples`, which never exceeds totalLatency.
	void delay(float *left, float *right, uint32_t frames, uint32_t samples)
	{
		const size_t size = delayLine[0].size();
		float *channels[] = {left, right};
		for (uint32_t i = 0; i < frames; ++i) {
			const size_t readPos = (writePos + size - samples) % size;
			for (size_t c = 0; c < 2; ++c) {
				delayLine[c][writePos] = channels[c][i];
				channels[c][i] = delayLine[c][readPos];
			}
			writePos = (writePos + 1) % size;
		}
	}

	std::array<Stretch, kWindowMs.size()> stretches;
	std::array<uint32_t, kWindowMs.size()> windowLatency{};
	std::array<std::vector<float>, 2> delayLine;
	std::array<std::array<float, 1024>, 2> scratch{};
	uint32_t totalLatency = 0;
	size_t writePos = 0;
	size_t previousWindow = 0;
	float previousSemitones = 0.0f;
	float previousFormant = 0.0f;
	bool resetPending = true;
};
