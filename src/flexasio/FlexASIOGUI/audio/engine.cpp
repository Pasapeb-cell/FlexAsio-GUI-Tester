#include "engine.h"

#include <pa_win_wasapi.h>

#include <QTimer>

#include "../../FlexASIOUtil/portaudio.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <windows.h>

namespace flexasio_gui {

	namespace {
		uint64_t TicksToMicroseconds(int64_t ticks) {
			static const int64_t frequency = [] {
				LARGE_INTEGER value{};
				QueryPerformanceFrequency(&value);
				return value.QuadPart;
			}();
			return uint64_t(ticks * 1'000'000 / frequency);
		}

		int64_t MicrosecondsToTicks(uint64_t microseconds) {
			static const int64_t frequency = [] {
				LARGE_INTEGER value{};
				QueryPerformanceFrequency(&value);
				return value.QuadPart;
			}();
			return int64_t(microseconds * uint64_t(frequency) / 1'000'000);
		}

		void UpdateMaximum(std::atomic<uint64_t>& target, uint64_t value) {
			auto observed = target.load(std::memory_order_relaxed);
			while (observed < value && !target.compare_exchange_weak(
				observed, value, std::memory_order_relaxed, std::memory_order_relaxed)) {}
		}

		PaWasapiFlags WasapiFlags(const TestStreamConfig& config) {
			PaWasapiFlags flags = static_cast<PaWasapiFlags>(0);
			if (config.wasapiExclusive) flags = PaWasapiFlags(flags | paWinWasapiExclusive);
			if (config.wasapiAutoConvert) flags = PaWasapiFlags(flags | paWinWasapiAutoConvert);
			if (config.wasapiExplicitSampleFormat) flags = PaWasapiFlags(flags | paWinWasapiExplicitSampleFormat);
			return flags;
		}

		std::string DescribeStream(const char* direction, const TestStreamConfig& config, double sampleRate) {
			return std::string(direction) + " device " + std::to_string(config.deviceIndex)
				+ ", " + std::to_string(config.channels) + " channels, "
				+ flexasio::GetSampleFormatString(config.sampleFormat) + ", "
				+ std::to_string(sampleRate) + " Hz";
		}
	}

	AudioEngine::AudioEngine(QObject* parent) : QObject(parent) {
		pollTimer = new QTimer(this);
		pollTimer->setInterval(100);
		connect(pollTimer, &QTimer::timeout, this, &AudioEngine::PollStats);
	}

	AudioEngine::~AudioEngine() {
		Stop();
	}

	void AudioEngine::Start(const TestConfig& config) {
		Stop();

		if (config.output.deviceIndex < 0 || config.output.channels <= 0)
			throw std::runtime_error("The Tester requires a valid enabled output device and positive output channel count.");
		if (config.bufferSizeSamples < 16 || config.bufferSizeSamples > 1024)
			throw std::runtime_error("Tester buffer size must be between 16 and 1024 samples.");
		if (!std::isfinite(config.sampleRate) || config.sampleRate <= 0)
			throw std::runtime_error("Sample rate must be a finite positive value.");

		const auto* outputDeviceInfo = Pa_GetDeviceInfo(config.output.deviceIndex);
		if (outputDeviceInfo == nullptr) throw std::runtime_error("Invalid output device selected");
		const auto* outputHostApiInfo = Pa_GetHostApiInfo(outputDeviceInfo->hostApi);
		if (outputHostApiInfo == nullptr) throw std::runtime_error("Unable to query host API for selected output device");
		if (config.input.has_value() && Pa_GetDeviceInfo(config.input->deviceIndex) == nullptr)
			throw std::runtime_error("Invalid input device selected");

		outputChannels = config.output.channels;
		outputSampleFormat = config.output.sampleFormat;
		sampleRate = config.sampleRate;
		stressPercent = std::clamp(config.stressPercent, 0, 90);
		signalBuffer = GenerateSignal(config.signal, config.sampleRate, outputChannels, config.volume);
		playbackPosition = 0;
		totalCallbacks.store(0, std::memory_order_relaxed);
		totalDropouts.store(0, std::memory_order_relaxed);
		peakLeft.store(0, std::memory_order_relaxed);
		peakRight.store(0, std::memory_order_relaxed);
		totalDeadlineWarnings.store(0, std::memory_order_relaxed);
		callbackWorkMicroseconds.store(0, std::memory_order_relaxed);
		callbackBudgetMicroseconds.store(0, std::memory_order_relaxed);
		worstCallbackLoadHundredths.store(0, std::memory_order_relaxed);
		worstCallbackJitterMicroseconds.store(0, std::memory_order_relaxed);
		lastCallbackStartTick.store(0, std::memory_order_relaxed);
		lastPolledCallbacks = 0;
		lastPolledDropouts = 0;

		PaStreamParameters outputParams{};
		outputParams.device = config.output.deviceIndex;
		outputParams.channelCount = config.output.channels;
		outputParams.sampleFormat = paNonInterleaved | config.output.sampleFormat;
		outputParams.suggestedLatency = config.output.suggestedLatencySeconds;
		PaWasapiStreamInfo outputWasapiInfo{};
		if (outputHostApiInfo->type == paWASAPI) {
			outputWasapiInfo.size = sizeof(outputWasapiInfo);
			outputWasapiInfo.hostApiType = paWASAPI;
			outputWasapiInfo.version = 1;
			outputWasapiInfo.flags = WasapiFlags(config.output);
			outputParams.hostApiSpecificStreamInfo = &outputWasapiInfo;
		}

		PaStreamParameters inputParams{};
		PaWasapiStreamInfo inputWasapiInfo{};
		const PaStreamParameters* inputParamsPtr = nullptr;
		if (config.input.has_value()) {
			const auto* inputDeviceInfo = Pa_GetDeviceInfo(config.input->deviceIndex);
			const auto* inputHostApiInfo = inputDeviceInfo == nullptr ? nullptr : Pa_GetHostApiInfo(inputDeviceInfo->hostApi);
			if (inputDeviceInfo == nullptr || inputHostApiInfo == nullptr || config.input->channels <= 0)
				throw std::runtime_error("The enabled input stream has an invalid device or channel count.");
			inputParams.device = config.input->deviceIndex;
			inputParams.channelCount = config.input->channels;
			inputParams.sampleFormat = paNonInterleaved | config.input->sampleFormat;
			inputParams.suggestedLatency = config.input->suggestedLatencySeconds;
			if (inputHostApiInfo->type == paWASAPI) {
				inputWasapiInfo.size = sizeof(inputWasapiInfo);
				inputWasapiInfo.hostApiType = paWASAPI;
				inputWasapiInfo.version = 1;
				inputWasapiInfo.flags = WasapiFlags(*config.input);
				inputParams.hostApiSpecificStreamInfo = &inputWasapiInfo;
			}
			inputParamsPtr = &inputParams;
		}

		const auto supported = Pa_IsFormatSupported(inputParamsPtr, &outputParams, config.sampleRate);
		if (supported != paFormatIsSupported) {
			const auto inputDescription = config.input.has_value()
				? DescribeStream("Input", *config.input, config.sampleRate) + "; " : "";
			throw std::runtime_error(inputDescription + DescribeStream("Output", config.output, config.sampleRate)
				+ " is not supported: " + Pa_GetErrorText(supported));
		}

		const auto openError = Pa_OpenStream(
			&stream,
			inputParamsPtr,
			&outputParams,
			config.sampleRate,
			(unsigned long)(config.bufferSizeSamples),
			paNoFlag,
			&AudioEngine::PaCallback,
			this);
		if (openError != paNoError) {
			stream = nullptr;
			throw std::runtime_error(std::string("Unable to open configured audio stream: ") + Pa_GetErrorText(openError));
		}

		const auto startError = Pa_StartStream(stream);
		if (startError != paNoError) {
			Pa_CloseStream(stream);
			stream = nullptr;
			throw std::runtime_error(std::string("Unable to start audio stream: ") + Pa_GetErrorText(startError));
		}
		const auto* streamInfo = Pa_GetStreamInfo(stream);
		actualInputLatencySeconds = streamInfo != nullptr ? streamInfo->inputLatency : 0.0;
		actualOutputLatencySeconds = streamInfo != nullptr ? streamInfo->outputLatency : 0.0;

		startTime = std::chrono::steady_clock::now();
		pollTimer->start();
		emit stateChanged(EngineState::Playing);
	}

	void AudioEngine::Stop() {
		if (stream == nullptr) return;
		pollTimer->stop();
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
		stream = nullptr;
		emit stateChanged(EngineState::Stopped);
	}

	void AudioEngine::PollStats() {
		const auto callbacks = totalCallbacks.load(std::memory_order_relaxed);
		const auto dropouts = totalDropouts.load(std::memory_order_relaxed);

		EngineStats stats;
		stats.totalCallbacks = callbacks;
		stats.totalDropouts = dropouts;
		stats.windowCallbacks = callbacks - lastPolledCallbacks;
		stats.windowDropouts = dropouts - lastPolledDropouts;
		stats.peakLeft = float(peakLeft.exchange(0, std::memory_order_relaxed)) / 1000.0f;
		stats.peakRight = float(peakRight.exchange(0, std::memory_order_relaxed)) / 1000.0f;
		const auto workUs = callbackWorkMicroseconds.exchange(0, std::memory_order_relaxed);
		const auto budgetUs = callbackBudgetMicroseconds.exchange(0, std::memory_order_relaxed);
		stats.averageCallbackLoadPercent = budgetUs > 0 ? 100.0 * double(workUs) / double(budgetUs) : 0.0;
		stats.worstCallbackLoadPercent = double(worstCallbackLoadHundredths.exchange(0, std::memory_order_relaxed)) / 100.0;
		stats.worstCallbackJitterMilliseconds = double(worstCallbackJitterMicroseconds.exchange(0, std::memory_order_relaxed)) / 1000.0;
		stats.totalDeadlineWarnings = totalDeadlineWarnings.load(std::memory_order_relaxed);
		stats.actualInputLatencySeconds = actualInputLatencySeconds;
		stats.actualOutputLatencySeconds = actualOutputLatencySeconds;
		stats.streamCpuLoadPercent = Pa_GetStreamCpuLoad(stream) * 100.0;
		stats.elapsedSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();

		lastPolledCallbacks = callbacks;
		lastPolledDropouts = dropouts;

		emit statsUpdated(stats);
	}

	int AudioEngine::PaCallback(
		const void*, void* output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo*,
		PaStreamCallbackFlags statusFlags,
		void* userData) {
		return static_cast<AudioEngine*>(userData)->RenderCallback(output, frameCount, statusFlags);
	}

	int AudioEngine::RenderCallback(void* output, unsigned long frameCount, PaStreamCallbackFlags statusFlags) {
		LARGE_INTEGER callbackStart{};
		QueryPerformanceCounter(&callbackStart);
		const uint64_t periodUs = uint64_t(std::llround(double(frameCount) / sampleRate * 1'000'000.0));
		const auto previousStart = lastCallbackStartTick.exchange(callbackStart.QuadPart, std::memory_order_relaxed);
		if (previousStart != 0) {
			const auto intervalUs = TicksToMicroseconds(callbackStart.QuadPart - previousStart);
			const auto jitterUs = intervalUs > periodUs ? intervalUs - periodUs : periodUs - intervalUs;
			UpdateMaximum(worstCallbackJitterMicroseconds, jitterUs);
		}

		totalCallbacks.fetch_add(1, std::memory_order_relaxed);
		if ((statusFlags & (paInputUnderflow | paOutputUnderflow)) != 0)
			totalDropouts.fetch_add(1, std::memory_order_relaxed);

		for (unsigned long frame = 0; frame < frameCount; ++frame) for (int channel = 0; channel < outputChannels; ++channel) {
			const float sample = NextSignalSample(channel);
			WriteOutputSample(output, frame, channel, sample);
			const int scaledPeak = int(std::clamp(std::abs(sample), 0.0f, 1.0f) * 1000.0f);
			auto& peak = channel == 0 ? peakLeft : peakRight;
			int observed = peak.load(std::memory_order_relaxed);
			while (observed < scaledPeak && !peak.compare_exchange_weak(
				observed, scaledPeak, std::memory_order_relaxed, std::memory_order_relaxed)) {}
		}

		// Deliberately consume a bounded portion of the callback budget, without any
		// allocation or blocking. This tests DSP-like pressure that a trivial signal
		// generator otherwise leaves unused.
		const auto stressDeadline = callbackStart.QuadPart + MicrosecondsToTicks(periodUs * uint64_t(stressPercent) / 100);
		if (stressPercent > 0) {
			LARGE_INTEGER now{};
			do {
				stressAccumulator = std::fmod(stressAccumulator * 1.61803398875 + 0.38196601125, 1.0);
				QueryPerformanceCounter(&now);
			} while (now.QuadPart < stressDeadline);
		}

		LARGE_INTEGER callbackEnd{};
		QueryPerformanceCounter(&callbackEnd);
		const auto workUs = TicksToMicroseconds(callbackEnd.QuadPart - callbackStart.QuadPart);
		callbackWorkMicroseconds.fetch_add(workUs, std::memory_order_relaxed);
		callbackBudgetMicroseconds.fetch_add(periodUs, std::memory_order_relaxed);
		const auto loadHundredths = periodUs > 0 ? workUs * 10'000 / periodUs : 0;
		UpdateMaximum(worstCallbackLoadHundredths, loadHundredths);
		if (loadHundredths >= 8'000) totalDeadlineWarnings.fetch_add(1, std::memory_order_relaxed);
		return paContinue;
	}

	float AudioEngine::NextSignalSample(int channel) {
		if (signalBuffer.empty()) return 0.0f;
		const auto index = (playbackPosition + size_t(channel)) % signalBuffer.size();
		const float sample = signalBuffer[index];
		if (channel == outputChannels - 1)
			playbackPosition = (playbackPosition + size_t(outputChannels)) % signalBuffer.size();
		return sample;
	}

	void AudioEngine::WriteOutputSample(void* output, unsigned long frame, int channel, float value) const {
		auto** channelBuffers = static_cast<void**>(output);
		switch (outputSampleFormat) {
		case paFloat32: static_cast<float*>(channelBuffers[channel])[frame] = value; break;
		case paInt32: static_cast<int32_t*>(channelBuffers[channel])[frame] = int32_t(std::lrint(std::clamp(value, -1.0f, 1.0f) * 2147483647.0f)); break;
		case paInt16: static_cast<int16_t*>(channelBuffers[channel])[frame] = int16_t(std::lrint(std::clamp(value, -1.0f, 1.0f) * 32767.0f)); break;
		case paInt24: {
			const int32_t sample = int32_t(std::lrint(std::clamp(value, -1.0f, 1.0f) * 8388607.0f));
			auto* bytes = static_cast<uint8_t*>(channelBuffers[channel]) + size_t(frame) * 3;
			bytes[0] = uint8_t(sample & 0xff); bytes[1] = uint8_t((sample >> 8) & 0xff); bytes[2] = uint8_t((sample >> 16) & 0xff);
			break;
		}
		default: break;
		}
	}

}
