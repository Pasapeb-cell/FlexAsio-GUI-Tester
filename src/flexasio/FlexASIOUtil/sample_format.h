#pragma once

#include <portaudio.h>

#include <optional>
#include <string>

namespace flexasio {

	// Resolves FlexASIO's four supported sample formats.  Keeping this in the
	// shared utility library prevents the driver and GUI tester from silently
	// choosing different formats for an otherwise identical WASAPI stream.
	PaSampleFormat ResolvePortAudioSampleFormat(
		PaHostApiTypeId hostApiType,
		PaDeviceIndex deviceIndex,
		const std::optional<std::string>& configuredSampleType,
		bool wasapiExclusiveMode);

}
