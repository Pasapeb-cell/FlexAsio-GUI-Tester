#pragma once

#include <QObject>

#include <portaudio.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

#include "signal_generator.h"

class QTimer;

namespace flexasio_gui {

	enum class EngineState { Stopped, Playing };

	struct EngineStats {
		uint64_t totalCallbacks = 0;
		uint64_t totalDropouts = 0;
		uint64_t windowCallbacks = 0;
		uint64_t windowDropouts = 0;
		double elapsedSeconds = 0;
	};

	struct TestConfig {
		int deviceIndex = -1;
		int channels = 2;
		double sampleRate = 48000;
		int64_t bufferSizeSamples = 256;
		double suggestedLatencySeconds = 0.0;
		bool wasapiExclusive = false;
		SignalType signal = SignalType::Sine440;
		double volume = 0.5;
	};

	// Plays a test signal through a PortAudio output stream and detects dropouts via
	// PortAudio's paOutputUnderflow status flag - the same authoritative, C-layer signal
	// FlexASIO itself would experience for the same device/backend/buffer size.
	//
	// The PortAudio callback runs on PortAudio's own high-priority audio thread. It only
	// ever touches plain atomics; a QTimer on the GUI thread polls them at 10 Hz to update
	// the UI, so no cross-thread Qt signal emission ever happens from the callback itself.
	class AudioEngine final : public QObject {
		Q_OBJECT
	public:
		explicit AudioEngine(QObject* parent = nullptr);
		~AudioEngine() override;

		AudioEngine(const AudioEngine&) = delete;
		AudioEngine& operator=(const AudioEngine&) = delete;

		// Throws std::runtime_error (message suitable for display) on failure, e.g. device
		// busy in WASAPI exclusive mode, or an unsupported sample rate/buffer size.
		void Start(const TestConfig& config);
		void Stop();
		bool IsPlaying() const { return stream != nullptr; }
		uint64_t TotalDropouts() const { return totalDropouts.load(std::memory_order_relaxed); }

	signals:
		void statsUpdated(flexasio_gui::EngineStats stats);
		void stateChanged(flexasio_gui::EngineState state);

	private slots:
		void PollStats();

	private:
		static int PaCallback(
			const void* input, void* output,
			unsigned long frameCount,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags,
			void* userData);
		int RenderCallback(void* output, unsigned long frameCount, PaStreamCallbackFlags statusFlags);

		PaStream* stream = nullptr;
		std::vector<float> signalBuffer;
		size_t playbackPosition = 0;
		int channels = 2;

		std::atomic<uint64_t> totalCallbacks{0};
		std::atomic<uint64_t> totalDropouts{0};

		uint64_t lastPolledCallbacks = 0;
		uint64_t lastPolledDropouts = 0;
		std::chrono::steady_clock::time_point startTime;

		QTimer* pollTimer = nullptr;
	};

}

Q_DECLARE_METATYPE(flexasio_gui::EngineStats)
Q_DECLARE_METATYPE(flexasio_gui::EngineState)
