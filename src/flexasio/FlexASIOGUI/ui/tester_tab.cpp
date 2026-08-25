#include "tester_tab.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include <optional>
#include <stdexcept>
#include <variant>

#include "../core/device_enumerator.h"
#include "../../FlexASIOUtil/sample_format.h"
#include "settings_tab.h"
#include "theme.h"
#include "widgets/angular_button.h"
#include "widgets/angular_panel.h"
#include "widgets/level_meter.h"
#include "widgets/quality_indicator.h"

namespace flexasio_gui {

	namespace {
		constexpr int kMinBufferSize = 16;
		// The tester is for quickly finding a low-latency stable value. Larger values
		// remain available in Settings, but 1024 is the useful upper bound here.
		constexpr int kMaxBufferSize = 1024;
		constexpr int kRestartDebounceMs = 150;

		std::optional<TestStreamConfig> BuildStreamConfig(
			const SettingsTab& settings, const flexasio::Config::Stream& stream, bool input, double sampleRate, int64_t bufferSize) {
			if (std::holds_alternative<flexasio::Config::NoDevice>(stream.device)) return std::nullopt;
			int deviceIndex = settings.SelectedDeviceIndex(input);
			if (deviceIndex < 0 && std::holds_alternative<flexasio::Config::DefaultDevice>(stream.device))
				deviceIndex = GetDefaultDevice(settings.SelectedHostApiIndex(), input ? Direction::Input : Direction::Output);
			if (deviceIndex < 0)
				throw std::runtime_error(std::string(input ? "Input" : "Output") + " device is not available. Choose an enumerated device or Default Device.");
			const auto* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			if (deviceInfo == nullptr) throw std::runtime_error("Selected audio device no longer exists.");
			const auto* hostApi = Pa_GetHostApiInfo(deviceInfo->hostApi);
			if (hostApi == nullptr) throw std::runtime_error("Unable to inspect selected audio device host API.");

			TestStreamConfig result;
			result.deviceIndex = deviceIndex;
			result.channels = stream.channels.value_or(input ? deviceInfo->maxInputChannels : deviceInfo->maxOutputChannels);
			result.suggestedLatencySeconds = stream.suggestedLatencySeconds.value_or(3.0 * double(bufferSize) / sampleRate);
			result.wasapiExclusive = stream.wasapiExclusiveMode;
			result.wasapiAutoConvert = stream.wasapiAutoConvert;
			result.wasapiExplicitSampleFormat = stream.wasapiExplicitSampleFormat;
			result.sampleFormat = flexasio::ResolvePortAudioSampleFormat(hostApi->type, deviceIndex, stream.sampleType, stream.wasapiExclusiveMode);
			return result;
		}
	}

	TesterTab::TesterTab(SettingsTab& settingsTab_, QWidget* parent)
		: QWidget(parent), settingsTab(settingsTab_), engine(), autoTuner(engine) {
		BuildUi();

		connect(&engine, &AudioEngine::statsUpdated, this, &TesterTab::OnStatsUpdated);
		connect(&engine, &AudioEngine::stateChanged, this, &TesterTab::OnStateChanged);
		connect(&autoTuner, &AutoTuner::progress, this, &TesterTab::OnAutoTuneProgress);
		connect(&autoTuner, &AutoTuner::finished, this, &TesterTab::OnAutoTuneFinished);
		connect(&autoTuner, &AutoTuner::failed, this, &TesterTab::OnAutoTuneFailed);
		connect(&settingsTab, &SettingsTab::configChanged, this, &TesterTab::OnBufferSizeChanged);
	}

	TesterTab::~TesterTab() {
		autoTuner.Cancel();
		engine.Stop();
	}

	void TesterTab::BuildUi() {
		setObjectName("TabPage");
		auto* rootLayout = new QVBoxLayout(this);
		rootLayout->setSpacing(12);

		auto* signalPanel = new AngularPanel("Test Signal");
		auto* topRow = new QHBoxLayout();
		topRow->setSpacing(10);
		topRow->addWidget(new QLabel("Waveform"));
		signalCombo = new QComboBox();
		signalCombo->addItems({"440 Hz Sine", "Pink Noise", "Sweep 20Hz-20kHz"});
		topRow->addWidget(signalCombo, 1);

		topRow->addSpacing(12);
		topRow->addWidget(new QLabel("Sample rate"));
		sampleRateCombo = new QComboBox();
		sampleRateCombo->addItems({"44100", "48000", "88200", "96000", "192000"});
		{
			const int idx = sampleRateCombo->findText(QString::number(int(settingsTab.SampleRateHint())));
			sampleRateCombo->setCurrentIndex(idx >= 0 ? idx : 1);
		}
		topRow->addWidget(sampleRateCombo);

		topRow->addSpacing(12);
		topRow->addWidget(new QLabel("Volume"));
		volumeSlider = new QSlider(Qt::Horizontal);
		volumeSlider->setRange(0, 100);
		volumeSlider->setValue(50);
		topRow->addWidget(volumeSlider, 1);

		topRow->addSpacing(12);
		topRow->addWidget(new QLabel("Stress"));
		stressCombo = new QComboBox();
		stressCombo->addItem("Idle", 0);
		stressCombo->addItem("Light (25%)", 25);
		stressCombo->addItem("Typical (50%)", 50);
		stressCombo->addItem("Heavy (70%)", 70);
		stressCombo->setToolTip("Controlled CPU work inside each audio callback. Use it to test headroom, not as a DAW simulation.");
		topRow->addWidget(stressCombo);
		signalPanel->ContentLayout()->addLayout(topRow);
		rootLayout->addWidget(signalPanel);

		auto* bufferPanel = new AngularPanel("Buffer Size / Quick Iteration");
		bufferPanel->SetAccent(theme::kAccent2);
		auto* bufferLayout = new QHBoxLayout();
		bufferLayout->setSpacing(10);
		bufferSizeSlider = new QSlider(Qt::Horizontal);
		bufferSizeSlider->setRange(kMinBufferSize, kMaxBufferSize);
		bufferSizeSpin = new QSpinBox();
		bufferSizeSpin->setRange(kMinBufferSize, kMaxBufferSize);
		bufferSizeSpin->setValue(int(settingsTab.BufferSizeSamples()));
		bufferSizeSpin->setSuffix(" smp");
		bufferSizeMsLabel = new QLabel();
		bufferSizeMsLabel->setFont(theme::DisplayFont(12, /*bold=*/true, /*letterSpacingPercent=*/112));
		bufferSizeMsLabel->setMinimumWidth(210);
		bufferSizeMsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		bufferLayout->addWidget(bufferSizeSlider, 1);
		bufferLayout->addWidget(bufferSizeSpin);
		bufferLayout->addWidget(bufferSizeMsLabel);
		bufferPanel->ContentLayout()->addLayout(bufferLayout);
		rootLayout->addWidget(bufferPanel);

		connect(bufferSizeSlider, &QSlider::valueChanged, bufferSizeSpin, &QSpinBox::setValue);
		connect(bufferSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), bufferSizeSlider, &QSlider::setValue);
		connect(bufferSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TesterTab::OnBufferSizeChanged);
		connect(sampleRateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TesterTab::OnBufferSizeChanged);
		connect(stressCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TesterTab::OnBufferSizeChanged);
		connect(signalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TesterTab::OnBufferSizeChanged);
		connect(volumeSlider, &QSlider::valueChanged, this, &TesterTab::OnBufferSizeChanged);

		auto* buttonsRow = new QHBoxLayout();
		buttonsRow->setSpacing(10);
		playButton = new AngularButton("Play");
		playButton->SetEmphasis(AngularButton::Emphasis::Primary);
		stopButton = new AngularButton("Stop");
		stopButton->SetEmphasis(AngularButton::Emphasis::Danger);
		autoTuneButton = new AngularButton("Auto-Tune");
		autoTuneModeCombo = new QComboBox();
		autoTuneModeCombo->addItem("Thorough", int(AutoTuneMode::Thorough));
		autoTuneModeCombo->addItem("Quick", int(AutoTuneMode::Quick));
		autoTuneModeCombo->setToolTip("Thorough: 10 seconds per candidate plus 60-second validation of the result and next-smaller size. Quick: 5 seconds per candidate.");
		applyButton = new AngularButton("Apply to FlexASIO.toml");
		stopButton->setEnabled(false);
		buttonsRow->addWidget(playButton);
		buttonsRow->addWidget(stopButton);
		buttonsRow->addWidget(autoTuneButton);
		buttonsRow->addWidget(autoTuneModeCombo);
		buttonsRow->addStretch(1);
		buttonsRow->addWidget(applyButton);
		rootLayout->addLayout(buttonsRow);

		connect(playButton, &AngularButton::clicked, this, &TesterTab::OnPlayClicked);
		connect(stopButton, &AngularButton::clicked, this, &TesterTab::OnStopClicked);
		connect(autoTuneButton, &AngularButton::clicked, this, &TesterTab::OnAutoTuneClicked);
		connect(applyButton, &AngularButton::clicked, this, [this] {
			settingsTab.SetBufferSizeSamples(bufferSizeSpin->value());
			settingsTab.SaveCurrentConfig();
		});

		auto* metricsPanel = new AngularPanel("Live Metrics");
		auto* metricsLayout = metricsPanel->ContentLayout();
		qualityIndicator = new QualityIndicator();
		metricsLayout->addWidget(qualityIndicator);
		levelMeter = new LevelMeter();
		metricsLayout->addWidget(levelMeter);

		auto* statsRow = new QHBoxLayout();
		statsRow->setSpacing(28);
		dropoutsLabel = new QLabel("Dropouts  -");
		glitchRateLabel = new QLabel("Glitch rate  -");
		elapsedLabel = new QLabel("Elapsed  -");
		for (auto* label : {dropoutsLabel, glitchRateLabel, elapsedLabel}) {
			label->setFont(theme::MonoFont(10));
			statsRow->addWidget(label);
		}
		statsRow->addStretch(1);
		metricsLayout->addLayout(statsRow);
		streamInfoLabel = new QLabel("Actual output latency  -");
		streamInfoLabel->setFont(theme::MonoFont(10));
		streamInfoLabel->setProperty("role", "dim");
		metricsLayout->addWidget(streamInfoLabel);
		callbackInfoLabel = new QLabel("Callback headroom  -");
		callbackInfoLabel->setFont(theme::MonoFont(10));
		callbackInfoLabel->setProperty("role", "dim");
		metricsLayout->addWidget(callbackInfoLabel);
		verdictLabel = new QLabel();
		verdictLabel->setFont(theme::MonoFont(10));
		verdictLabel->setProperty("role", "dim");
		metricsLayout->addWidget(verdictLabel);

		autoTuneStatusLabel = new QLabel();
		autoTuneStatusLabel->setFont(theme::MonoFont(10));
		autoTuneStatusLabel->setProperty("role", "dim");
		metricsLayout->addWidget(autoTuneStatusLabel);

		rootLayout->addWidget(metricsPanel);
		rootLayout->addStretch(1);

		restartDebounceTimer = new QTimer(this);
		restartDebounceTimer->setSingleShot(true);
		connect(restartDebounceTimer, &QTimer::timeout, this, &TesterTab::RestartIfPlaying);

		UpdateBufferSizeMsLabel();
		SetVerdict(TestVerdict::NotTested);
	}

	TestConfig TesterTab::BuildTestConfig() const {
		const double sampleRate = sampleRateCombo->currentText().toDouble();
		const int64_t bufferSize = bufferSizeSpin->value();
		const auto flexConfig = settingsTab.CurrentConfig();
		TestConfig config;
		config.input = BuildStreamConfig(settingsTab, flexConfig.input, /*input=*/true, sampleRate, bufferSize);
		const auto output = BuildStreamConfig(settingsTab, flexConfig.output, /*input=*/false, sampleRate, bufferSize);
		if (!output.has_value()) throw std::runtime_error("Output is disabled in FlexASIO.toml; enable an output stream before using the Tester.");
		config.output = *output;
		config.sampleRate = sampleRate;
		config.bufferSizeSamples = bufferSize;
		config.volume = volumeSlider->value() / 100.0;
		config.stressPercent = stressCombo->currentData().toInt();

		switch (signalCombo->currentIndex()) {
		case 0: config.signal = SignalType::Sine440; break;
		case 1: config.signal = SignalType::PinkNoise; break;
		default: config.signal = SignalType::Sweep; break;
		}
		return config;
	}

	void TesterTab::UpdateBufferSizeMsLabel() {
		const double sampleRate = sampleRateCombo->currentText().toDouble();
		const double ms = sampleRate > 0 ? double(bufferSizeSpin->value()) / sampleRate * 1000.0 : 0.0;
		bufferSizeMsLabel->setText(QString("%1 samples = %2 ms").arg(bufferSizeSpin->value()).arg(ms, 0, 'f', 2));
	}

	void TesterTab::OnBufferSizeChanged() {
		UpdateBufferSizeMsLabel();
		if (engine.IsPlaying()) restartDebounceTimer->start(kRestartDebounceMs);
	}

	void TesterTab::RestartIfPlaying() {
		if (!engine.IsPlaying()) return;
		try {
			engine.Start(BuildTestConfig());
		}
		catch (const std::exception& exception) {
			ShowError(exception.what());
		}
	}

	void TesterTab::OnPlayClicked() {
		try {
			engine.Start(BuildTestConfig());
		}
		catch (const std::exception& exception) {
			ShowError(exception.what());
		}
	}

	void TesterTab::OnStopClicked() {
		if (autoTuneRunning) {
			autoTuner.Cancel();
			autoTuneRunning = false;
			autoTuneStatusLabel->setText("Auto-tune cancelled.");
			playButton->setEnabled(true);
			autoTuneButton->setEnabled(true);
			return;
		}
		engine.Stop();
	}

	void TesterTab::OnAutoTuneClicked() {
		try {
			autoTuneRunning = true;
			playButton->setEnabled(false);
			autoTuneButton->setEnabled(false);
			autoTuneStatusLabel->setText("Starting auto-tune...");
			autoTuner.Start(BuildTestConfig(), AutoTuneMode(autoTuneModeCombo->currentData().toInt()));
		}
		catch (const std::exception& exception) {
			autoTuneRunning = false;
			playButton->setEnabled(true);
			autoTuneButton->setEnabled(true);
			ShowError(exception.what());
		}
	}

	void TesterTab::OnStatsUpdated(EngineStats stats) {
		dropoutsLabel->setText(QString("Dropouts  %1").arg(stats.totalDropouts));
		const double rate = stats.totalCallbacks > 0 ? 100.0 * double(stats.totalDropouts) / double(stats.totalCallbacks) : 0.0;
		glitchRateLabel->setText(QString("Glitch rate  %1%").arg(rate, 0, 'f', 2));
		elapsedLabel->setText(QString("Elapsed  %1s").arg(stats.elapsedSeconds, 0, 'f', 1));
		streamInfoLabel->setText(QString("Actual latency  in %1 ms / out %2 ms   |   Stream CPU  %3%")
			.arg(stats.actualInputLatencySeconds * 1000.0, 0, 'f', 2)
			.arg(stats.actualOutputLatencySeconds * 1000.0, 0, 'f', 2)
			.arg(stats.streamCpuLoadPercent, 0, 'f', 1));
		callbackInfoLabel->setText(QString("Callback load  avg %1% / worst %2%   |   jitter %3 ms   |   deadline warnings %4")
			.arg(stats.averageCallbackLoadPercent, 0, 'f', 1)
			.arg(stats.worstCallbackLoadPercent, 0, 'f', 1)
			.arg(stats.worstCallbackJitterMilliseconds, 0, 'f', 3)
			.arg(stats.totalDeadlineWarnings));

		if (stats.totalDropouts > 0) SetVerdict(TestVerdict::UnderflowDetected);
		else if (stats.totalDeadlineWarnings > 0) SetVerdict(TestVerdict::HeadroomLimited);
		else SetVerdict(TestVerdict::CallbackStable);
		levelMeter->SetPeaks(stats.peakLeft, stats.peakRight);
	}

	void TesterTab::OnStateChanged(EngineState state) {
		const bool playing = state == EngineState::Playing;
		playButton->setEnabled(!playing && !autoTuneRunning);
		stopButton->setEnabled(playing || autoTuneRunning);
		if (!playing && !autoTuneRunning) levelMeter->SetPeaks(0.0f, 0.0f);
	}

	void TesterTab::OnAutoTuneProgress(qint64 bufferSize, int index, int count) {
		autoTuneStatusLabel->setText(QString("Testing %1 samples (%2/%3)...").arg(bufferSize).arg(index + 1).arg(count));
		QSignalBlocker blocker(bufferSizeSpin);
		bufferSizeSpin->setValue(int(bufferSize));
		UpdateBufferSizeMsLabel();
	}

	void TesterTab::OnAutoTuneFinished(qint64 minimumStableBufferSize) {
		autoTuneRunning = false;
		playButton->setEnabled(true);
		autoTuneButton->setEnabled(true);
		bufferSizeSpin->setValue(int(minimumStableBufferSize));
		autoTuneStatusLabel->setText(QString("Minimum stable buffer size: %1 samples.").arg(minimumStableBufferSize));
	}

	void TesterTab::OnAutoTuneFailed(const QString& reason) {
		autoTuneRunning = false;
		playButton->setEnabled(true);
		autoTuneButton->setEnabled(true);
		autoTuneStatusLabel->setText("Auto-tune could not test this device.");
		ShowError(reason);
	}

	void TesterTab::ShowError(const QString& message) {
		SetVerdict(TestVerdict::OpenFailed);
		QMessageBox::warning(this, "Audio Tester Error", message);
	}

	void TesterTab::SetVerdict(TestVerdict newVerdict) {
		verdict = newVerdict;
		QString text;
		QualityLevel level = QualityLevel::Unknown;
		switch (verdict) {
		case TestVerdict::CallbackStable: text = "Verdict: Callback stable — not end-to-end hardware validation."; level = QualityLevel::Green; break;
		case TestVerdict::HeadroomLimited: text = "Verdict: Headroom limited — callbacks reached 80% of their deadline."; level = QualityLevel::Yellow; break;
		case TestVerdict::UnderflowDetected: text = "Verdict: Underflow detected by PortAudio."; level = QualityLevel::Red; break;
		case TestVerdict::OpenFailed: text = "Verdict: Stream open or format check failed."; level = QualityLevel::Red; break;
		case TestVerdict::NotTested:
		default: text = "Verdict: Not tested."; break;
		}
		qualityIndicator->SetLevel(level);
		verdictLabel->setText(text);
	}

}
