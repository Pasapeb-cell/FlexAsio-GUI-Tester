#pragma once

#include <QWidget>

#include "../audio/auto_tuner.h"
#include "../audio/engine.h"

class QComboBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;

namespace flexasio_gui {

	class SettingsTab;
	class QualityIndicator;

	// The audio tester: plays a test signal through the device/backend currently selected
	// in SettingsTab, at an independently adjustable buffer size, and shows live dropout
	// metrics via PortAudio's own underflow detection - so the user can find a stable
	// buffer size without touching their DAW.
	class TesterTab final : public QWidget {
		Q_OBJECT
	public:
		explicit TesterTab(SettingsTab& settingsTab, QWidget* parent = nullptr);
		~TesterTab() override;

	private:
		void BuildUi();
		TestConfig BuildTestConfig() const;
		void UpdateBufferSizeMsLabel();

		void OnPlayClicked();
		void OnStopClicked();
		void OnAutoTuneClicked();
		void OnBufferSizeChanged();
		void RestartIfPlaying();
		void OnStatsUpdated(EngineStats stats);
		void OnStateChanged(EngineState state);
		void OnAutoTuneProgress(qint64 bufferSize, int index, int count);
		void OnAutoTuneFinished(qint64 minimumStableBufferSize);
		void ShowError(const QString& message);

		SettingsTab& settingsTab;
		AudioEngine engine;
		AutoTuner autoTuner;

		QComboBox* signalCombo = nullptr;
		QComboBox* sampleRateCombo = nullptr;
		QSlider* volumeSlider = nullptr;
		QSlider* bufferSizeSlider = nullptr;
		QSpinBox* bufferSizeSpin = nullptr;
		QLabel* bufferSizeMsLabel = nullptr;

		QPushButton* playButton = nullptr;
		QPushButton* stopButton = nullptr;
		QPushButton* autoTuneButton = nullptr;
		QPushButton* applyButton = nullptr;

		QualityIndicator* qualityIndicator = nullptr;
		QLabel* dropoutsLabel = nullptr;
		QLabel* glitchRateLabel = nullptr;
		QLabel* elapsedLabel = nullptr;
		QLabel* autoTuneStatusLabel = nullptr;

		QTimer* restartDebounceTimer = nullptr;
		bool autoTuneRunning = false;
	};

}
