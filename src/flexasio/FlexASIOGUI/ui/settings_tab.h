#pragma once

#include <QWidget>

#include <cstdint>

#include "../../FlexASIO/config.h"

class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QSlider;
class QFileSystemWatcher;
class QTimer;

namespace flexasio_gui {

	class AngularPanel;

	// Editor for FlexASIO.toml: backend, buffer size, and per-direction (input/output)
	// device, channel, sample type, latency, and WASAPI options.
	//
	// Fields where FlexASIO's "unset" behavior differs meaningfully from any concrete value
	// (sampleType auto-detection, suggestedLatencySeconds defaulting to 3x buffer length)
	// keep an explicit "Auto" option so a user who never touches them doesn't silently have
	// that auto-behavior replaced by a fixed value on save. Backend, buffer size, and
	// channels don't have this issue (their UI defaults already match FlexASIO's own
	// defaults), so those are always written explicitly.
	class SettingsTab final : public QWidget {
		Q_OBJECT
	public:
		explicit SettingsTab(QWidget* parent = nullptr);

		flexasio::Config CurrentConfig() const;

		int SelectedHostApiIndex() const;
		// Device index for the given direction, or -1 if Default/Disabled/regex is selected.
		int SelectedDeviceIndex(bool input) const;
		int64_t BufferSizeSamples() const;
		bool WasapiExclusive(bool input) const;
		// Best-guess sample rate for the Latency/Tester tabs to default to.
		double SampleRateHint() const;

		// Used by the Tester tab's "Apply to FlexASIO.toml" action.
		void SetBufferSizeSamples(int64_t value);
		void SaveCurrentConfig();

	signals:
		// Emitted whenever any control changes, so other tabs can refresh their view.
		void configChanged();

	private:
		struct StreamControls {
			AngularPanel* panel = nullptr;
			QComboBox* deviceCombo = nullptr;
			QSpinBox* channelsSpin = nullptr;
			QComboBox* sampleTypeCombo = nullptr;
			QDoubleSpinBox* suggestedLatencySpin = nullptr;
			QCheckBox* wasapiExclusiveCheck = nullptr;
			QCheckBox* wasapiAutoConvertCheck = nullptr;
			QCheckBox* wasapiExplicitFormatCheck = nullptr;
		};

		void BuildUi();
		AngularPanel* BuildStreamGroup(const QString& title, StreamControls& controls);
		void RefreshDeviceLists();
		void RefreshDeviceListForStream(StreamControls& controls, bool input);
		void UpdateChannelsDefaultForDevice(StreamControls& controls);
		void RefreshWasapiControlsVisibility();
		void ApplyConfigToUi(const flexasio::Config& config);
		void ApplyStreamToUi(const flexasio::Config::Stream& stream, StreamControls& controls);
		flexasio::Config::Stream StreamFromUi(const StreamControls& controls) const;
		void ReloadFromDisk();

		void OnSaveClicked();
		void OnResetClicked();

		// Watches FlexASIO.toml for changes made outside this application (e.g. a text
		// editor, or another instance of this GUI) and offers to reload when it happens.
		void SetupConfigWatcher();
		void OnConfigFileMaybeChanged();
		void RearmConfigFileWatch();

		QComboBox* backendCombo = nullptr;
		QSlider* bufferSizeSlider = nullptr;
		QSpinBox* bufferSizeSpin = nullptr;

		StreamControls inputControls;
		StreamControls outputControls;

		QFileSystemWatcher* configWatcher = nullptr;
		QTimer* configWatchDebounceTimer = nullptr;
		bool suppressNextConfigWatchPrompt = false;
	};

}
