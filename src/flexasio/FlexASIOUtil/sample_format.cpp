#include "sample_format.h"

#include "portaudio.h"

#include <ks.h>
#include <ksmedia.h>

#include <stdexcept>

namespace flexasio {

	namespace {
		PaSampleFormat ParseSampleType(const std::string& value) {
			if (value == "Float32") return paFloat32;
			if (value == "Int32") return paInt32;
			if (value == "Int24") return paInt24;
			if (value == "Int16") return paInt16;
			throw std::runtime_error("Invalid '" + value + "' sample type; valid values are Float32, Int32, Int24, and Int16");
		}

		PaSampleFormat FromWaveFormat(const WAVEFORMATEXTENSIBLE& format) {
			const auto validBits = format.Samples.wValidBitsPerSample != 0
				? format.Samples.wValidBitsPerSample : format.Format.wBitsPerSample;
			if (format.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT && validBits == 32) return paFloat32;
			if (format.SubFormat == KSDATAFORMAT_SUBTYPE_PCM && validBits == 32) return paInt32;
			if (format.SubFormat == KSDATAFORMAT_SUBTYPE_PCM && validBits == 24) return paInt24;
			if (format.SubFormat == KSDATAFORMAT_SUBTYPE_PCM && validBits == 16) return paInt16;
			throw std::runtime_error("WASAPI device default format is not one of FlexASIO's supported sample types");
		}
	}

	PaSampleFormat ResolvePortAudioSampleFormat(
		PaHostApiTypeId hostApiType,
		PaDeviceIndex deviceIndex,
		const std::optional<std::string>& configuredSampleType,
		bool wasapiExclusiveMode) {
		if (configuredSampleType.has_value()) return ParseSampleType(*configuredSampleType);
		if (hostApiType == paWASAPI && wasapiExclusiveMode) {
			try {
				return FromWaveFormat(GetWasapiDeviceDefaultFormat(deviceIndex));
			}
			catch (const std::exception&) {
				// FlexASIO deliberately falls back to Float32 when a device does not
				// expose a usable default format.  The tester follows that behavior.
			}
		}
		return paFloat32;
	}

}
