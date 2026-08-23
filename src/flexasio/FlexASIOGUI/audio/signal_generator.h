#pragma once

#include <vector>

namespace flexasio_gui {

	enum class SignalType { Sine440, PinkNoise, Sweep };

	// Pre-generates a short, seamlessly-loopable buffer of interleaved float32 samples in
	// [-amplitude, amplitude]. The audio engine loops this buffer during playback so its
	// real-time callback never has to synthesize samples itself.
	std::vector<float> GenerateSignal(SignalType type, double sampleRate, int channels, double amplitude);

}
