#pragma once

#include <string>
#include <vector>

namespace flexasio_gui {

	struct HostApiInfo {
		int index = 0;
		std::string name;
	};

	struct DeviceInfo {
		int index = 0;
		int hostApiIndex = 0;
		std::string name;
		int maxInputChannels = 0;
		int maxOutputChannels = 0;
		double defaultSampleRate = 0;
		double defaultLowInputLatency = 0;
		double defaultHighInputLatency = 0;
		double defaultLowOutputLatency = 0;
		double defaultHighOutputLatency = 0;
	};

	enum class Direction { Input, Output };

	// RAII wrapper around Pa_Initialize()/Pa_Terminate(). One instance must be kept alive
	// for the lifetime of the process wherever device enumeration or audio streaming happens.
	class PortAudioSession final {
	public:
		PortAudioSession();
		~PortAudioSession();
		PortAudioSession(const PortAudioSession&) = delete;
		PortAudioSession& operator=(const PortAudioSession&) = delete;
	};

	// Host API names match exactly what FlexASIO expects in its `backend` config field
	// (e.g. "Windows WASAPI"), since both use the same PortAudio build.
	std::vector<HostApiInfo> GetHostApis();

	// Devices belonging to the given host API with a nonzero channel count for `direction`.
	// Device names match exactly what FlexASIO expects in its `device` config field.
	std::vector<DeviceInfo> GetDevices(int hostApiIndex, Direction direction);

	// The host API's default device for the given direction, or -1 if unavailable.
	int GetDefaultDevice(int hostApiIndex, Direction direction);

}
