#include "latency_calculator.h"

namespace flexasio_gui {

	namespace {
		constexpr double kSharedBackendOverheadMs = 15.0;

		bool BypassesWindowsAudioEngine(const std::string& backendName, bool wasapiExclusive) {
			if (backendName == "Windows WDM-KS") return true;
			if (backendName == "Windows WASAPI" && wasapiExclusive) return true;
			return false;
		}
	}

	LatencyBreakdown ComputeLatency(
		int64_t bufferSizeSamples,
		double sampleRate,
		std::optional<double> suggestedLatencySeconds,
		const std::string& backendName,
		bool wasapiExclusive) {
		LatencyBreakdown result;
		if (sampleRate <= 0) return result;

		result.bufferLatencyMs = double(bufferSizeSamples) / sampleRate * 1000.0;
		result.suggestedLatencyMs = suggestedLatencySeconds.has_value()
			? *suggestedLatencySeconds * 1000.0
			: 3.0 * result.bufferLatencyMs;
		result.backendOverheadMs = BypassesWindowsAudioEngine(backendName, wasapiExclusive) ? 0.0 : kSharedBackendOverheadMs;
		result.totalLatencyMs = result.bufferLatencyMs + result.suggestedLatencyMs + result.backendOverheadMs;
		return result;
	}

	std::string StabilityRiskLabel(int64_t bufferSizeSamples, double sampleRate) {
		if (sampleRate <= 0) return "Unknown";
		const double bufferMs = double(bufferSizeSamples) / sampleRate * 1000.0;
		if (bufferMs < 1.0) return "Very High";
		if (bufferMs < 2.0) return "High";
		if (bufferMs < 5.0) return "Medium";
		if (bufferMs < 10.0) return "Low";
		if (bufferMs < 20.0) return "Very Low";
		return "Negligible";
	}

}
