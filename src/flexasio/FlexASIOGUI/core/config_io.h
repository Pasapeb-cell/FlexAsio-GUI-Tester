#pragma once

#include <filesystem>

#include "../../FlexASIO/config.h"

namespace flexasio_gui {

	// Path to the FlexASIO.toml file that FlexASIO reads its configuration from
	// (%USERPROFILE%\FlexASIO.toml).
	std::filesystem::path GetConfigPath();

	// Loads the current FlexASIO.toml, reusing FlexASIO's own parser. Returns a
	// default-constructed Config if the file does not exist yet.
	flexasio::Config LoadConfig();

	// Serializes config to FlexASIO's expected TOML format and writes it to GetConfigPath(),
	// overwriting any existing file. tinytoml (FlexASIO's TOML library) only supports
	// reading, so this covers exactly the fields config.cpp knows how to parse.
	void SaveConfig(const flexasio::Config& config);

}
