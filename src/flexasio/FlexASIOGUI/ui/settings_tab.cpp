#include "settings_tab.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include <variant>

#include "../core/config_io.h"
#include "../core/device_enumerator.h"
#include "theme.h"
#include "widgets/angular_button.h"
#include "widgets/angular_panel.h"

namespace flexasio_gui {

	namespace {
		constexpr int kDeviceDataDefault = -1;
		constexpr int kDeviceDataDisabled = -2;
		constexpr int kDeviceDataLiteral = -3; // configured device name not currently enumerable
		constexpr int kDeviceDataRegex = -4;
		constexpr int kMaxChannelsRole = Qt::UserRole + 2;
		constexpr int kRegexPatternRole = Qt::UserRole + 1;

		constexpr int64_t kMinBufferSize = 8;
		constexpr int64_t kMaxBufferSize = 1024;
	}

	SettingsTab::SettingsTab(QWidget* parent) : QWidget(parent) {
		BuildUi();
		if (!QCoreApplication::instance()->property("flexasioGuiSmokeTest").toBool()) {
			ReloadFromDisk();
			SetupConfigWatcher();
		}
	}

	void SettingsTab::BuildUi() {
		setObjectName("TabPage");
		auto* rootLayout = new QVBoxLayout(this);
		rootLayout->setSpacing(12);

		auto* driverPanel = new AngularPanel("Driver");
		auto* topRow = new QHBoxLayout();
		topRow->setSpacing(10);
		topRow->addWidget(new QLabel("Backend"));
		backendCombo = new QComboBox();
		for (const auto& hostApi : GetHostApis())
			backendCombo->addItem(QString::fromStdString(hostApi.name), hostApi.index);
		topRow->addWidget(backendCombo, 1);

		topRow->addSpacing(16);
		topRow->addWidget(new QLabel("Buffer size"));
		bufferSizeSlider = new QSlider(Qt::Horizontal);
		bufferSizeSlider->setRange(int(kMinBufferSize), int(kMaxBufferSize));
		bufferSizeSpin = new QSpinBox();
		bufferSizeSpin->setRange(int(kMinBufferSize), int(kMaxBufferSize));
		bufferSizeSpin->setValue(256);
		bufferSizeSpin->setSuffix(" smp");
		topRow->addWidget(bufferSizeSlider, 2);
		topRow->addWidget(bufferSizeSpin);
		driverPanel->ContentLayout()->addLayout(topRow);
		rootLayout->addWidget(driverPanel);

		connect(bufferSizeSlider, &QSlider::valueChanged, bufferSizeSpin, &QSpinBox::setValue);
		connect(bufferSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), bufferSizeSlider, &QSlider::setValue);
		connect(bufferSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsTab::configChanged);

		auto* streamsRow = new QHBoxLayout();
		streamsRow->setSpacing(12);
		streamsRow->addWidget(BuildStreamGroup("Input", inputControls));
		streamsRow->addWidget(BuildStreamGroup("Output", outputControls));
		rootLayout->addLayout(streamsRow, 1);

		auto* buttonsRow = new QHBoxLayout();
		buttonsRow->setSpacing(10);
		auto* saveButton = new AngularButton("Save to FlexASIO.toml");
		saveButton->SetEmphasis(AngularButton::Emphasis::Primary);
		auto* resetButton = new AngularButton("Reset Defaults");
		buttonsRow->addStretch(1);
		buttonsRow->addWidget(resetButton);
		buttonsRow->addWidget(saveButton);
		rootLayout->addLayout(buttonsRow);

		connect(saveButton, &AngularButton::clicked, this, &SettingsTab::OnSaveClicked);
		connect(resetButton, &AngularButton::clicked, this, &SettingsTab::OnResetClicked);

		connect(backendCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
			RefreshDeviceLists();
			RefreshWasapiControlsVisibility();
			emit configChanged();
		});

		if (QCoreApplication::instance()->property("flexasioGuiSmokeTest").toBool()) {
			for (auto* controls : {&inputControls, &outputControls}) {
				controls->deviceCombo->addItem("Default Device", kDeviceDataDefault);
				controls->deviceCombo->addItem("Disabled", kDeviceDataDisabled);
			}
		}
		else RefreshDeviceLists();
		RefreshWasapiControlsVisibility();
	}

	AngularPanel* SettingsTab::BuildStreamGroup(const QString& title, StreamControls& controls) {
		auto* panel = new AngularPanel(title);
		// Output is the direction the tester exercises, so give it the magenta accent to
		// distinguish it at a glance from input.
		if (title == "Output") panel->SetAccent(theme::kAccent2);
		controls.panel = panel;
		auto* layout = panel->ContentLayout();

		auto* deviceRow = new QHBoxLayout();
		deviceRow->addWidget(new QLabel("Device"));
		controls.deviceCombo = new QComboBox();
		deviceRow->addWidget(controls.deviceCombo, 1);
		layout->addLayout(deviceRow);

		auto* channelsRow = new QHBoxLayout();
		channelsRow->addWidget(new QLabel("Channels"));
		controls.channelsSpin = new QSpinBox();
		controls.channelsSpin->setRange(1, 256);
		controls.channelsSpin->setValue(2);
		channelsRow->addWidget(controls.channelsSpin);

		channelsRow->addSpacing(12);
		channelsRow->addWidget(new QLabel("Sample type"));
		controls.sampleTypeCombo = new QComboBox();
		controls.sampleTypeCombo->addItems({"Auto", "Float32", "Int32", "Int24", "Int16"});
		channelsRow->addWidget(controls.sampleTypeCombo, 1);
		layout->addLayout(channelsRow);

		auto* latencyRow = new QHBoxLayout();
		latencyRow->addWidget(new QLabel("Suggested latency"));
		controls.suggestedLatencySpin = new QDoubleSpinBox();
		controls.suggestedLatencySpin->setDecimals(3);
		controls.suggestedLatencySpin->setSingleStep(0.001);
		controls.suggestedLatencySpin->setRange(-0.001, 3600.0);
		controls.suggestedLatencySpin->setSuffix(" s");
		controls.suggestedLatencySpin->setSpecialValueText("Auto (3x buffer)");
		latencyRow->addWidget(controls.suggestedLatencySpin, 1);
		layout->addLayout(latencyRow);

		auto* wasapiRow = new QHBoxLayout();
		controls.wasapiExclusiveCheck = new QCheckBox("WASAPI Exclusive");
		controls.wasapiAutoConvertCheck = new QCheckBox("Auto Convert");
		controls.wasapiExplicitFormatCheck = new QCheckBox("Explicit Sample Format");
		controls.wasapiAutoConvertCheck->setChecked(true);
		controls.wasapiExplicitFormatCheck->setChecked(true);
		wasapiRow->addWidget(controls.wasapiExclusiveCheck);
		wasapiRow->addWidget(controls.wasapiAutoConvertCheck);
		wasapiRow->addWidget(controls.wasapiExplicitFormatCheck);
		wasapiRow->addStretch(1);
		layout->addLayout(wasapiRow);

		layout->addStretch(1);

		StreamControls* controlsPtr = &controls;
		connect(controls.deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, controlsPtr](int) {
			UpdateChannelsDefaultForDevice(*controlsPtr);
			emit configChanged();
		});
		connect(controls.channelsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsTab::configChanged);
		connect(controls.sampleTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsTab::configChanged);
		connect(controls.suggestedLatencySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SettingsTab::configChanged);
		connect(controls.wasapiExclusiveCheck, &QCheckBox::toggled, this, &SettingsTab::configChanged);
		connect(controls.wasapiAutoConvertCheck, &QCheckBox::toggled, this, &SettingsTab::configChanged);
		connect(controls.wasapiExplicitFormatCheck, &QCheckBox::toggled, this, &SettingsTab::configChanged);

		return panel;
	}

	void SettingsTab::RefreshDeviceLists() {
		RefreshDeviceListForStream(inputControls, /*input=*/true);
		RefreshDeviceListForStream(outputControls, /*input=*/false);
	}

	void SettingsTab::RefreshDeviceListForStream(StreamControls& controls, bool input) {
		QComboBox* combo = controls.deviceCombo;
		const QString previousText = combo->count() > 0 ? combo->currentText() : QString();

		combo->blockSignals(true);
		combo->clear();
		combo->addItem("Default Device", kDeviceDataDefault);
		combo->addItem("Disabled", kDeviceDataDisabled);

		const int hostApiIndex = SelectedHostApiIndex();
		if (hostApiIndex >= 0) {
			try {
				for (const auto& device : GetDevices(hostApiIndex, input ? Direction::Input : Direction::Output)) {
					combo->addItem(QString::fromStdString(device.name), device.index);
					combo->setItemData(combo->count() - 1, input ? device.maxInputChannels : device.maxOutputChannels, kMaxChannelsRole);
				}
			}
			catch (const std::exception&) {
				// Leave the list with just Default/Disabled if enumeration fails.
			}
		}

		const int restoreIndex = previousText.isEmpty() ? 0 : combo->findText(previousText);
		combo->setCurrentIndex(restoreIndex >= 0 ? restoreIndex : 0);
		combo->blockSignals(false);

		UpdateChannelsDefaultForDevice(controls);
	}

	void SettingsTab::UpdateChannelsDefaultForDevice(StreamControls& controls) {
		const int idx = controls.deviceCombo->currentIndex();
		if (idx < 0) return;
		const int data = controls.deviceCombo->itemData(idx).toInt();
		if (data < 0) return;
		const int maxChannels = controls.deviceCombo->itemData(idx, kMaxChannelsRole).toInt();
		if (maxChannels <= 0) return;
		controls.channelsSpin->setMaximum(maxChannels);
		controls.channelsSpin->setValue(maxChannels);
	}

	void SettingsTab::RefreshWasapiControlsVisibility() {
		const bool isWasapi = backendCombo->currentText() == "Windows WASAPI";
		for (StreamControls* controls : {&inputControls, &outputControls}) {
			controls->wasapiExclusiveCheck->setVisible(isWasapi);
			controls->wasapiAutoConvertCheck->setVisible(isWasapi);
			controls->wasapiExplicitFormatCheck->setVisible(isWasapi);
		}
	}

	void SettingsTab::ApplyConfigToUi(const flexasio::Config& config) {
		if (config.backend.has_value()) {
			const int idx = backendCombo->findText(QString::fromStdString(*config.backend));
			backendCombo->setCurrentIndex(idx >= 0 ? idx : 0);
		}
		else {
			backendCombo->setCurrentIndex(0);
		}
		RefreshDeviceLists();
		RefreshWasapiControlsVisibility();

		bufferSizeSpin->setValue(int(config.bufferSizeSamples.value_or(256)));

		ApplyStreamToUi(config.input, inputControls);
		ApplyStreamToUi(config.output, outputControls);
	}

	void SettingsTab::ApplyStreamToUi(const flexasio::Config::Stream& stream, StreamControls& controls) {
		QComboBox* combo = controls.deviceCombo;
		int selectIndex = 0;
		std::visit([&](const auto& value) {
			using T = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<T, flexasio::Config::DefaultDevice>) {
				selectIndex = combo->findData(kDeviceDataDefault);
			}
			else if constexpr (std::is_same_v<T, flexasio::Config::NoDevice>) {
				selectIndex = combo->findData(kDeviceDataDisabled);
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				int idx = -1;
				for (int i = 0; i < combo->count(); ++i) {
					if (combo->itemData(i).toInt() >= 0 && combo->itemText(i).toStdString() == value) { idx = i; break; }
				}
				if (idx < 0) {
					combo->addItem(QString::fromStdString(value), kDeviceDataLiteral);
					idx = combo->count() - 1;
				}
				selectIndex = idx;
			}
			else if constexpr (std::is_same_v<T, flexasio::Config::DeviceRegex>) {
				const QString pattern = QString::fromStdString(value.getString());
				combo->addItem("(regex) " + pattern, kDeviceDataRegex);
				combo->setItemData(combo->count() - 1, pattern, kRegexPatternRole);
				selectIndex = combo->count() - 1;
			}
		}, stream.device);
		combo->setCurrentIndex(selectIndex >= 0 ? selectIndex : 0);
		UpdateChannelsDefaultForDevice(controls);

		if (stream.channels.has_value()) controls.channelsSpin->setValue(*stream.channels);

		if (stream.sampleType.has_value()) {
			const int idx = controls.sampleTypeCombo->findText(QString::fromStdString(*stream.sampleType));
			controls.sampleTypeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
		}
		else {
			controls.sampleTypeCombo->setCurrentIndex(0);
		}

		if (stream.suggestedLatencySeconds.has_value())
			controls.suggestedLatencySpin->setValue(*stream.suggestedLatencySeconds);
		else
			controls.suggestedLatencySpin->setValue(controls.suggestedLatencySpin->minimum());

		controls.wasapiExclusiveCheck->setChecked(stream.wasapiExclusiveMode);
		controls.wasapiAutoConvertCheck->setChecked(stream.wasapiAutoConvert);
		controls.wasapiExplicitFormatCheck->setChecked(stream.wasapiExplicitSampleFormat);
	}

	flexasio::Config::Stream SettingsTab::StreamFromUi(const StreamControls& controls) const {
		flexasio::Config::Stream stream;

		const int idx = controls.deviceCombo->currentIndex();
		const int data = idx >= 0 ? controls.deviceCombo->itemData(idx).toInt() : kDeviceDataDefault;
		if (data == kDeviceDataDefault) stream.device = flexasio::Config::DefaultDevice();
		else if (data == kDeviceDataDisabled) stream.device = flexasio::Config::NoDevice();
		else if (data == kDeviceDataRegex)
			stream.device = flexasio::Config::DeviceRegex(controls.deviceCombo->itemData(idx, kRegexPatternRole).toString().toStdString());
		else
			stream.device = controls.deviceCombo->itemText(idx).toStdString();

		stream.channels = controls.channelsSpin->value();

		if (controls.sampleTypeCombo->currentIndex() > 0)
			stream.sampleType = controls.sampleTypeCombo->currentText().toStdString();

		if (controls.suggestedLatencySpin->value() >= 0.0)
			stream.suggestedLatencySeconds = controls.suggestedLatencySpin->value();

		stream.wasapiExclusiveMode = controls.wasapiExclusiveCheck->isChecked();
		stream.wasapiAutoConvert = controls.wasapiAutoConvertCheck->isChecked();
		stream.wasapiExplicitSampleFormat = controls.wasapiExplicitFormatCheck->isChecked();

		return stream;
	}

	flexasio::Config SettingsTab::CurrentConfig() const {
		flexasio::Config config;
		if (backendCombo->count() > 0) config.backend = backendCombo->currentText().toStdString();
		config.bufferSizeSamples = int64_t(bufferSizeSpin->value());
		config.input = StreamFromUi(inputControls);
		config.output = StreamFromUi(outputControls);
		return config;
	}

	int SettingsTab::SelectedHostApiIndex() const {
		if (backendCombo->count() == 0) return -1;
		return backendCombo->currentData().toInt();
	}

	int SettingsTab::SelectedDeviceIndex(bool input) const {
		const StreamControls& controls = input ? inputControls : outputControls;
		const int idx = controls.deviceCombo->currentIndex();
		if (idx < 0) return -1;
		const int data = controls.deviceCombo->itemData(idx).toInt();
		return data >= 0 ? data : -1;
	}

	int64_t SettingsTab::BufferSizeSamples() const { return int64_t(bufferSizeSpin->value()); }

	bool SettingsTab::WasapiExclusive(bool input) const {
		const StreamControls& controls = input ? inputControls : outputControls;
		return controls.wasapiExclusiveCheck->isChecked();
	}

	double SettingsTab::SampleRateHint() const {
		const int hostApiIndex = SelectedHostApiIndex();
		const int deviceIndex = SelectedDeviceIndex(/*input=*/false);
		if (hostApiIndex >= 0 && deviceIndex >= 0) {
			try {
				for (const auto& device : GetDevices(hostApiIndex, Direction::Output))
					if (device.index == deviceIndex) return device.defaultSampleRate;
			}
			catch (const std::exception&) {}
		}
		return 48000.0;
	}

	void SettingsTab::ReloadFromDisk() {
		try {
			ApplyConfigToUi(LoadConfig());
		}
		catch (const std::exception& exception) {
			QMessageBox::warning(this, "Unable to load FlexASIO.toml",
				QString("Using default settings instead.\n\n%1").arg(exception.what()));
			ApplyConfigToUi(flexasio::Config());
		}
	}

	void SettingsTab::SetBufferSizeSamples(int64_t value) {
		bufferSizeSpin->setValue(int(value));
	}

	void SettingsTab::SaveCurrentConfig() {
		try {
			// Our own write is about to make FlexASIO.toml change out from under the
			// watcher too - suppress the one prompt that would otherwise cause.
			suppressNextConfigWatchPrompt = true;
			SaveConfig(CurrentConfig());
			RearmConfigFileWatch();
			QMessageBox::information(this, "Saved", "Settings written to FlexASIO.toml.");
		}
		catch (const std::exception& exception) {
			suppressNextConfigWatchPrompt = false;
			QMessageBox::critical(this, "Unable to save FlexASIO.toml", exception.what());
		}
	}

	void SettingsTab::SetupConfigWatcher() {
		configWatcher = new QFileSystemWatcher(this);
		RearmConfigFileWatch();
		const auto dir = QString::fromStdWString(GetConfigPath().parent_path().wstring());
		if (!dir.isEmpty()) configWatcher->addPath(dir);

		configWatchDebounceTimer = new QTimer(this);
		configWatchDebounceTimer->setSingleShot(true);
		connect(configWatchDebounceTimer, &QTimer::timeout, this, &SettingsTab::OnConfigFileMaybeChanged);

		// Debounce: some editors write a file by deleting it and recreating it, or by
		// truncating then rewriting, which can otherwise fire multiple change events for a
		// single logical save.
		auto scheduleCheck = [this](const QString&) { configWatchDebounceTimer->start(300); };
		connect(configWatcher, &QFileSystemWatcher::fileChanged, this, scheduleCheck);
		connect(configWatcher, &QFileSystemWatcher::directoryChanged, this, scheduleCheck);
	}

	void SettingsTab::RearmConfigFileWatch() {
		const auto filePath = QString::fromStdWString(GetConfigPath().wstring());
		if (QFile::exists(filePath) && !configWatcher->files().contains(filePath))
			configWatcher->addPath(filePath);
	}

	void SettingsTab::OnConfigFileMaybeChanged() {
		// The file may have been deleted and recreated (dropping it from the watcher's
		// file list, which only the directory watch would have caught) - re-arm before
		// deciding whether to prompt.
		RearmConfigFileWatch();

		if (suppressNextConfigWatchPrompt) {
			suppressNextConfigWatchPrompt = false;
			return;
		}

		const auto reply = QMessageBox::question(this, "FlexASIO.toml changed",
			"FlexASIO.toml was modified outside this application. Reload it now?\n\n"
			"(Any unsaved changes in this window will be lost.)",
			QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::Yes) ReloadFromDisk();
	}

	void SettingsTab::OnSaveClicked() { SaveCurrentConfig(); }

	void SettingsTab::OnResetClicked() {
		ApplyConfigToUi(flexasio::Config());
	}

}
