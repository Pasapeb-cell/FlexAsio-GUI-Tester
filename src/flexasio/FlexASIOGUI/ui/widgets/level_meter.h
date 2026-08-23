#pragma once

#include <QWidget>

namespace flexasio_gui {

	// Callback-fed stereo peak meter. Values are received on the GUI thread from
	// AudioEngine's polling signal, never directly from PortAudio's audio thread.
	class LevelMeter final : public QWidget {
		Q_OBJECT
	public:
		explicit LevelMeter(QWidget* parent = nullptr);
		void SetPeaks(float left, float right);
		QSize sizeHint() const override;

	protected:
		void paintEvent(QPaintEvent*) override;

	private:
		float leftPeak = 0.0f;
		float rightPeak = 0.0f;
	};

}
