#include "engine.h"

#include <pa_win_wasapi.h>

#include <QTimer>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace flexasio_gui {

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

		const auto* deviceInfo = Pa_GetDeviceInfo(config.deviceIndex);
		if (deviceInfo == nullptr) throw std::runtime_error("Invalid audio device selected");
		const auto* hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
		if (hostApiInfo == nullptr) throw std::runtime_error("Unable to query host API for selected device");

		channels = config.channels;
		signalBuffer = GenerateSignal(config.signal, config.sampleRate, config.channels, config.volume);
		playbackPosition = 0;
		totalCallbacks.store(0, std::memory_order_relaxed);
		totalDropouts.store(0, std::memory_order_relaxed);
		peakLeft.store(0, std::memory_order_relaxed);
		peakRight.store(0, std::memory_order_relaxed);
		lastPolledCallbacks = 0;
		lastPolledDropouts = 0;

		PaStreamParameters outputParams{};
		outputParams.device = config.deviceIndex;
		outputParams.channelCount = config.channels;
		outputParams.sampleFormat = paFloat32;
		outputParams.suggestedLatency = config.suggestedLatencySeconds;

		PaWasapiStreamInfo wasapiInfo{};
		if (hostApiInfo->type == paWASAPI && config.wasapiExclusive) {
			wasapiInfo.size = sizeof(PaWasapiStreamInfo);
			wasapiInfo.hostApiType = paWASAPI;
			wasapiInfo.version = 1;
			wasapiInfo.flags = paWinWasapiExclusive;
			outputParams.hostApiSpecificStreamInfo = &wasapiInfo;
		}

		const auto openError = Pa_OpenStream(
			&stream,
			/*inputParameters=*/nullptr,
			&outputParams,
			config.sampleRate,
			(unsigned long)(config.bufferSizeSamples),
			paNoFlag,
			&AudioEngine::PaCallback,
			this);
		if (openError != paNoError) {
			stream = nullptr;
			throw std::runtime_error(std::string("Unable to open audio stream: ") + Pa_GetErrorText(openError));
		}

		const auto startError = Pa_StartStream(stream);
		if (startError != paNoError) {
			Pa_CloseStream(stream);
			stream = nullptr;
			throw std::runtime_error(std::string("Unable to start audio stream: ") + Pa_GetErrorText(startError));
		}
		const auto* streamInfo = Pa_GetStreamInfo(stream);
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
		totalCallbacks.fetch_add(1, std::memory_order_relaxed);
		if ((statusFlags & paOutputUnderflow) != 0)
			totalDropouts.fetch_add(1, std::memory_order_relaxed);

		auto* out = static_cast<float*>(output);
		const size_t samplesNeeded = size_t(frameCount) * size_t(channels);
		if (signalBuffer.empty()) {
			std::fill(out, out + samplesNeeded, 0.0f);
			return paContinue;
		}
		for (size_t i = 0; i < samplesNeeded; ++i) {
			out[i] = signalBuffer[playbackPosition];
			playbackPosition = (playbackPosition + 1) % signalBuffer.size();

			const int scaledPeak = int(std::clamp(std::abs(out[i]), 0.0f, 1.0f) * 1000.0f);
			// The MVP emits the same signal on each channel, but retain distinct meters
			// so future multi-channel generators can report their real left/right peaks.
			auto& peak = i % size_t(channels) == 0 ? peakLeft : peakRight;
			int observed = peak.load(std::memory_order_relaxed);
			while (observed < scaledPeak && !peak.compare_exchange_weak(
				observed, scaledPeak, std::memory_order_relaxed, std::memory_order_relaxed)) {}
		}
		return paContinue;
	}

}
