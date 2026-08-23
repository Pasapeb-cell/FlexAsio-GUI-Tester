#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace flexasio_gui {

	struct LatencyBreakdown {
		double bufferLatencyMs = 0;
		double suggestedLatencyMs = 0;
		double backendOverheadMs = 0;
		double totalLatencyMs = 0;
	};

	// Mirrors FlexASIO's own latency model (see flexasio.cpp ComputeBufferSizes() /
	// GetDefaultSuggestedLatency()): buffer latency is bufferSizeSamples/sampleRate, and
	// FlexASIO defaults suggestedLatencySeconds to 3x the buffer length when unset.
	// backendOverheadMs approximates the Windows audio engine's internal mixing buffer,
	// which WASAPI Exclusive and WDM-KS bypass entirely.
	LatencyBreakdown ComputeLatency(
		int64_t bufferSizeSamples,
		double sampleRate,
		std::optional<double> suggestedLatencySeconds,
		const std::string& backendName,
		bool wasapiExclusive);

	// Qualitative dropout-risk label ("Very High" .. "Negligible") for a buffer size at a
	// given sample rate, for use in the buffer size comparison table.
	std::string StabilityRiskLabel(int64_t bufferSizeSamples, double sampleRate);

}
