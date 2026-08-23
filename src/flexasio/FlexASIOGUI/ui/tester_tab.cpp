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

#include "../core/device_enumerator.h"
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
	}

	TesterTab::TesterTab(SettingsTab& settingsTab_, QWidget* parent)
		: QWidget(parent), settingsTab(settingsTab_), engine(), autoTuner(engine) {
		BuildUi();

		connect(&engine, &AudioEngine::statsUpdated, this, &TesterTab::OnStatsUpdated);
		connect(&engine, &AudioEngine::stateChanged, this, &TesterTab::OnStateChanged);
		connect(&autoTuner, &AutoTuner::progress, this, &TesterTab::OnAutoTuneProgress);
		connect(&autoTuner, &AutoTuner::finished, this, &TesterTab::OnAutoTuneFinished);
		connect(&autoTuner, &AutoTuner::failed, this, &TesterTab::OnAutoTuneFailed);
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

		auto* buttonsRow = new QHBoxLayout();
		buttonsRow->setSpacing(10);
		playButton = new AngularButton("Play");
		playButton->SetEmphasis(AngularButton::Emphasis::Primary);
		stopButton = new AngularButton("Stop");
		stopButton->SetEmphasis(AngularButton::Emphasis::Danger);
		autoTuneButton = new AngularButton("Auto-Tune");
		applyButton = new AngularButton("Apply to FlexASIO.toml");
		stopButton->setEnabled(false);
		buttonsRow->addWidget(playButton);
		buttonsRow->addWidget(stopButton);
		buttonsRow->addWidget(autoTuneButton);
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
	}

	TestConfig TesterTab::BuildTestConfig() const {
		TestConfig config;
		config.deviceIndex = settingsTab.SelectedDeviceIndex(/*input=*/false);
		if (config.deviceIndex < 0) {
			const int hostApiIndex = settingsTab.SelectedHostApiIndex();
			config.deviceIndex = hostApiIndex >= 0 ? GetDefaultDevice(hostApiIndex, Direction::Output) : -1;
		}
		config.channels = 2;
		config.sampleRate = sampleRateCombo->currentText().toDouble();
		config.bufferSizeSamples = bufferSizeSpin->value();
		config.suggestedLatencySeconds = 0.0;
		config.wasapiExclusive = settingsTab.WasapiExclusive(/*input=*/false);
		config.volume = volumeSlider->value() / 100.0;

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
		autoTuneRunning = true;
		playButton->setEnabled(false);
		autoTuneButton->setEnabled(false);
		autoTuneStatusLabel->setText("Starting auto-tune...");
		autoTuner.Start(BuildTestConfig());
	}

	void TesterTab::OnStatsUpdated(EngineStats stats) {
		dropoutsLabel->setText(QString("Dropouts  %1").arg(stats.totalDropouts));
		const double rate = stats.totalCallbacks > 0 ? 100.0 * double(stats.totalDropouts) / double(stats.totalCallbacks) : 0.0;
		glitchRateLabel->setText(QString("Glitch rate  %1%").arg(rate, 0, 'f', 2));
		elapsedLabel->setText(QString("Elapsed  %1s").arg(stats.elapsedSeconds, 0, 'f', 1));
		streamInfoLabel->setText(QString("Actual output latency  %1 ms   |   Stream CPU  %2%")
			.arg(stats.actualOutputLatencySeconds * 1000.0, 0, 'f', 2)
			.arg(stats.streamCpuLoadPercent, 0, 'f', 1));

		QualityLevel level;
		if (stats.totalDropouts == 0) level = QualityLevel::Green;
		else if (rate < 1.0) level = QualityLevel::Yellow;
		else level = QualityLevel::Red;
		qualityIndicator->SetLevel(level);
		levelMeter->SetPeaks(stats.peakLeft, stats.peakRight);
	}

	void TesterTab::OnStateChanged(EngineState state) {
		const bool playing = state == EngineState::Playing;
		playButton->setEnabled(!playing && !autoTuneRunning);
		stopButton->setEnabled(playing || autoTuneRunning);
		if (!playing && !autoTuneRunning) qualityIndicator->SetLevel(QualityLevel::Unknown);
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
		QMessageBox::warning(this, "Audio Tester Error", message);
	}

}
