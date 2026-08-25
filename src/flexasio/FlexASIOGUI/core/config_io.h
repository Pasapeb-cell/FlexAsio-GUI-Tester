#pragma once

#include <filesystem>
#include <string>

#include "../../FlexASIO/config.h"

namespace flexasio_gui {

	// Path to the FlexASIO.toml file that FlexASIO reads its configuration from
	// (%USERPROFILE%\FlexASIO.toml).
	std::filesystem::path GetConfigPath();

	// Loads the current FlexASIO.toml, reusing FlexASIO's own parser. Returns a
	// default-constructed Config if the file does not exist yet.
	flexasio::Config LoadConfig();

	// These are intentionally separate for deterministic unit tests and to ensure
	// persistence never writes unchecked generated TOML to the live configuration.
	std::string SerializeConfig(const flexasio::Config& config);
	void ValidateConfig(const flexasio::Config& config);

	// Serializes config to FlexASIO's expected TOML format, validates it through
	// FlexASIO's own loader, then atomically replaces the live configuration.
	void SaveConfig(const flexasio::Config& config);
	void SaveConfigToPath(const flexasio::Config& config, const std::filesystem::path& path);

}
