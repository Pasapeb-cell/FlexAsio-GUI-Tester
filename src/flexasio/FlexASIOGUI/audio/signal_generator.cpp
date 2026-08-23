#include "signal_generator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace flexasio_gui {

	namespace {
		constexpr double kPi = 3.14159265358979323846;

		std::vector<float> InterleaveMono(const std::vector<float>& mono, int channels) {
			std::vector<float> result(mono.size() * size_t(channels));
			for (size_t i = 0; i < mono.size(); ++i)
				for (int c = 0; c < channels; ++c)
					result[i * size_t(channels) + size_t(c)] = mono[i];
			return result;
		}

		std::vector<float> GenerateSine(double sampleRate, double amplitude) {
			constexpr double frequency = 440.0;
			// Loop over an integer number of cycles close to 0.5s so the buffer loops seamlessly
			// (no discontinuity, hence no audible click, at the wrap-around point).
			const double cycles = std::max(1.0, std::round(0.5 * frequency));
			const double duration = cycles / frequency;
			const size_t sampleCount = size_t(std::llround(duration * sampleRate));
			std::vector<float> mono(sampleCount);
			for (size_t i = 0; i < sampleCount; ++i)
				mono[i] = float(amplitude * std::sin(2.0 * kPi * frequency * double(i) / sampleRate));
			return mono;
		}

		std::vector<float> GeneratePinkNoise(double sampleRate, double amplitude) {
			const size_t sampleCount = size_t(std::llround(2.0 * sampleRate));
			std::vector<float> mono(sampleCount);

			// Voss-McCartney pink noise approximation: sum kRows white-noise generators, each
			// updated at half the rate of the previous one.
			constexpr int kRows = 16;
			std::array<double, kRows> rows{};
			std::mt19937 rng(12345);
			std::uniform_real_distribution<double> dist(-1.0, 1.0);
			double runningSum = 0;
			for (size_t i = 0; i < sampleCount; ++i) {
				int rowToUpdate = 0;
				size_t n = i + 1;
				while ((n & 1) == 0 && rowToUpdate < kRows - 1) { n >>= 1; ++rowToUpdate; }
				runningSum -= rows[size_t(rowToUpdate)];
				rows[size_t(rowToUpdate)] = dist(rng);
				runningSum += rows[size_t(rowToUpdate)];
				mono[i] = float(amplitude * (runningSum / kRows));
			}
			return mono;
		}

		std::vector<float> GenerateSweep(double sampleRate, double amplitude) {
			constexpr double startFreq = 20.0;
			constexpr double endFreq = 20000.0;
			constexpr double duration = 5.0;
			const size_t sampleCount = size_t(std::llround(duration * sampleRate));
			std::vector<float> mono(sampleCount);
			const double k = std::log(endFreq / startFreq) / duration;
			for (size_t i = 0; i < sampleCount; ++i) {
				const double t = double(i) / sampleRate;
				const double phase = 2.0 * kPi * startFreq / k * (std::exp(k * t) - 1.0);
				mono[i] = float(amplitude * std::sin(phase));
			}
			return mono;
		}
	}

	std::vector<float> GenerateSignal(SignalType type, double sampleRate, int channels, double amplitude) {
		std::vector<float> mono;
		switch (type) {
		case SignalType::Sine440: mono = GenerateSine(sampleRate, amplitude); break;
		case SignalType::PinkNoise: mono = GeneratePinkNoise(sampleRate, amplitude); break;
		case SignalType::Sweep: mono = GenerateSweep(sampleRate, amplitude); break;
		}
		return InterleaveMono(mono, channels);
	}

}
