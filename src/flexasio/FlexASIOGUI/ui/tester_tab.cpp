#include "tester_tab.h"

#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include "../core/device_enumerator.h"
#include "settings_tab.h"
#include "widgets/quality_indicator.h"

namespace flexasio_gui {

	namespace {
		constexpr int kMinBufferSize = 16;
		constexpr int kMaxBufferSize = 8192;
		constexpr int kRestartDebounceMs = 150;
	}

	TesterTab::TesterTab(SettingsTab& settingsTab_, QWidget* parent)
		: QWidget(parent), settingsTab(settingsTab_), engine(), autoTuner(engine) {
		BuildUi();

		connect(&engine, &AudioEngine::statsUpdated, this, &TesterTab::OnStatsUpdated);
		connect(&engine, &AudioEngine::stateChanged, this, &TesterTab::OnStateChanged);
		connect(&autoTuner, &AutoTuner::progress, this, &TesterTab::OnAutoTuneProgress);
		connect(&autoTuner, &AutoTuner::finished, this, &TesterTab::OnAutoTuneFinished);
	}

	TesterTab::~TesterTab() {
		autoTuner.Cancel();
		engine.Stop();
	}

	void TesterTab::BuildUi() {
		auto* rootLayout = new QVBoxLayout(this);

		auto* topRow = new QHBoxLayout();
		topRow->addWidget(new QLabel("Signal:"));
		signalCombo = new QComboBox();
		signalCombo->addItems({"440 Hz Sine", "Pink Noise", "Sweep 20Hz-20kHz"});
		topRow->addWidget(signalCombo);

		topRow->addWidget(new QLabel("Sample rate:"));
		sampleRateCombo = new QComboBox();
		sampleRateCombo->addItems({"44100", "48000", "88200", "96000", "192000"});
		{
			const int idx = sampleRateCombo->findText(QString::number(int(settingsTab.SampleRateHint())));
			sampleRateCombo->setCurrentIndex(idx >= 0 ? idx : 1);
		}
		topRow->addWidget(sampleRateCombo);

		topRow->addWidget(new QLabel("Volume:"));
		volumeSlider = new QSlider(Qt::Horizontal);
		volumeSlider->setRange(0, 100);
		volumeSlider->setValue(50);
		topRow->addWidget(volumeSlider, 1);
		rootLayout->addLayout(topRow);

		auto* bufferGroup = new QGroupBox("Buffer Size (Quick Iteration)");
		auto* bufferLayout = new QHBoxLayout(bufferGroup);
		bufferSizeSlider = new QSlider(Qt::Horizontal);
		bufferSizeSlider->setRange(kMinBufferSize, kMaxBufferSize);
		bufferSizeSpin = new QSpinBox();
		bufferSizeSpin->setRange(kMinBufferSize, kMaxBufferSize);
		bufferSizeSpin->setValue(int(settingsTab.BufferSizeSamples()));
		bufferSizeMsLabel = new QLabel();
		bufferLayout->addWidget(bufferSizeSlider, 1);
		bufferLayout->addWidget(bufferSizeSpin);
		bufferLayout->addWidget(bufferSizeMsLabel);
		rootLayout->addWidget(bufferGroup);

		connect(bufferSizeSlider, &QSlider::valueChanged, bufferSizeSpin, &QSpinBox::setValue);
		connect(bufferSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), bufferSizeSlider, &QSlider::setValue);
		connect(bufferSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &TesterTab::OnBufferSizeChanged);
		connect(sampleRateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TesterTab::OnBufferSizeChanged);

		auto* buttonsRow = new QHBoxLayout();
		playButton = new QPushButton("Play");
		stopButton = new QPushButton("Stop");
		autoTuneButton = new QPushButton("Auto-Tune: Find Minimum Stable Size");
		applyButton = new QPushButton("Apply to FlexASIO.toml");
		stopButton->setEnabled(false);
		buttonsRow->addWidget(playButton);
		buttonsRow->addWidget(stopButton);
		buttonsRow->addWidget(autoTuneButton);
		buttonsRow->addStretch(1);
		buttonsRow->addWidget(applyButton);
		rootLayout->addLayout(buttonsRow);

		connect(playButton, &QPushButton::clicked, this, &TesterTab::OnPlayClicked);
		connect(stopButton, &QPushButton::clicked, this, &TesterTab::OnStopClicked);
		connect(autoTuneButton, &QPushButton::clicked, this, &TesterTab::OnAutoTuneClicked);
		connect(applyButton, &QPushButton::clicked, this, [this] {
			settingsTab.SetBufferSizeSamples(bufferSizeSpin->value());
			settingsTab.SaveCurrentConfig();
		});

		auto* metricsGroup = new QGroupBox("Real-Time Metrics");
		auto* metricsLayout = new QVBoxLayout(metricsGroup);
		qualityIndicator = new QualityIndicator();
		dropoutsLabel = new QLabel("Dropouts: -");
		glitchRateLabel = new QLabel("Glitch rate: -");
		elapsedLabel = new QLabel("Elapsed: -");
		autoTuneStatusLabel = new QLabel();
		metricsLayout->addWidget(qualityIndicator);
		metricsLayout->addWidget(dropoutsLabel);
		metricsLayout->addWidget(glitchRateLabel);
		metricsLayout->addWidget(elapsedLabel);
		metricsLayout->addWidget(autoTuneStatusLabel);
		rootLayout->addWidget(metricsGroup);
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
		dropoutsLabel->setText(QString("Dropouts: %1").arg(stats.totalDropouts));
		const double rate = stats.totalCallbacks > 0 ? 100.0 * double(stats.totalDropouts) / double(stats.totalCallbacks) : 0.0;
		glitchRateLabel->setText(QString("Glitch rate: %1%").arg(rate, 0, 'f', 2));
		elapsedLabel->setText(QString("Elapsed: %1s").arg(stats.elapsedSeconds, 0, 'f', 1));

		QualityLevel level;
		if (stats.totalDropouts == 0) level = QualityLevel::Green;
		else if (rate < 1.0) level = QualityLevel::Yellow;
		else level = QualityLevel::Red;
		qualityIndicator->SetLevel(level);
	}

	void TesterTab::OnStateChanged(EngineState state) {
		const bool playing = state == EngineState::Playing;
		playButton->setEnabled(!playing && !autoTuneRunning);
		stopButton->setEnabled(playing || autoTuneRunning);
		if (!playing && !autoTuneRunning) qualityIndicator->SetLevel(QualityLevel::Unknown);
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

	void TesterTab::ShowError(const QString& message) {
		QMessageBox::warning(this, "Audio Tester Error", message);
	}

}
