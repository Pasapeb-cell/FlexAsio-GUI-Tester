#include "device_enumerator.h"

#include <portaudio.h>

#include <stdexcept>

namespace flexasio_gui {

	namespace {
		void ThrowOnPaError(PaError error) {
			if (error < 0) throw std::runtime_error(std::string("PortAudio error: ") + Pa_GetErrorText(error));
		}
	}

	PortAudioSession::PortAudioSession() { ThrowOnPaError(Pa_Initialize()); }
	PortAudioSession::~PortAudioSession() { Pa_Terminate(); }

	std::vector<HostApiInfo> GetHostApis() {
		std::vector<HostApiInfo> result;
		const auto count = Pa_GetHostApiCount();
		if (count < 0) ThrowOnPaError(count);
		for (PaHostApiIndex i = 0; i < count; ++i) {
			const auto* info = Pa_GetHostApiInfo(i);
			if (info == nullptr) continue;
			result.push_back(HostApiInfo{int(i), info->name});
		}
		return result;
	}

	std::vector<DeviceInfo> GetDevices(int hostApiIndex, Direction direction) {
		std::vector<DeviceInfo> result;
		const auto count = Pa_GetDeviceCount();
		if (count < 0) ThrowOnPaError(count);
		for (PaDeviceIndex i = 0; i < count; ++i) {
			const auto* info = Pa_GetDeviceInfo(i);
			if (info == nullptr) continue;
			if (int(info->hostApi) != hostApiIndex) continue;
			const auto relevantChannels = direction == Direction::Input ? info->maxInputChannels : info->maxOutputChannels;
			if (relevantChannels <= 0) continue;

			DeviceInfo device;
			device.index = int(i);
			device.hostApiIndex = int(info->hostApi);
			device.name = info->name;
			device.maxInputChannels = info->maxInputChannels;
			device.maxOutputChannels = info->maxOutputChannels;
			device.defaultSampleRate = info->defaultSampleRate;
			device.defaultLowInputLatency = info->defaultLowInputLatency;
			device.defaultHighInputLatency = info->defaultHighInputLatency;
			device.defaultLowOutputLatency = info->defaultLowOutputLatency;
			device.defaultHighOutputLatency = info->defaultHighOutputLatency;
			result.push_back(std::move(device));
		}
		return result;
	}

	int GetDefaultDevice(int hostApiIndex, Direction direction) {
		const auto* info = Pa_GetHostApiInfo(hostApiIndex);
		if (info == nullptr) return -1;
		const auto deviceIndex = direction == Direction::Input ? info->defaultInputDevice : info->defaultOutputDevice;
		return deviceIndex == paNoDevice ? -1 : int(deviceIndex);
	}

}
