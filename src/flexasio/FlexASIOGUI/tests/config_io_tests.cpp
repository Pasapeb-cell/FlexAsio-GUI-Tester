#include <QCoreApplication>
#include <QTemporaryDir>

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "../core/config_io.h"

namespace {
	void Require(bool value, const char* message) {
		if (!value) throw std::runtime_error(message);
	}
}

int main(int argc, char** argv) {
	QCoreApplication app(argc, argv);
	try {
		QTemporaryDir directory;
		Require(directory.isValid(), "unable to create temporary directory");
		const auto configPath = std::filesystem::path(directory.filePath("FlexASIO.toml").toStdWString());

		flexasio::Config config;
		config.bufferSizeSamples = 64;
		config.input.device = std::string(reinterpret_cast<const char*>(u8"Mic \"A\" \\ 東京\n"));
		config.input.channels = 1;
		config.input.suggestedLatencySeconds = 0.0; // Regression: this must serialize as 0.0, not 0.
		config.input.sampleType = "Int24";
		config.output.device = std::string(reinterpret_cast<const char*>(u8"Speakers\tControl"));
		config.output.channels = 2;
		config.output.sampleType = "Float32";

		const auto text = flexasio_gui::SerializeConfig(config);
		Require(text.find("suggestedLatencySeconds = 0.0000000000000000") != std::string::npos, "0.0 latency was not serialized as TOML float");
		flexasio_gui::SaveConfigToPath(config, configPath);
		const auto parsed = flexasio::LoadConfigFile(configPath);
		Require(parsed == config, "configuration round trip changed values");

		config.bufferSizeSamples = 1025;
		bool rejected = false;
		try { flexasio_gui::ValidateConfig(config); }
		catch (const std::exception&) { rejected = true; }
		Require(rejected, "out-of-range buffer was accepted");
	}
	catch (const std::exception& exception) {
		std::cerr << exception.what() << '\n';
		return 1;
	}
	return 0;
}
