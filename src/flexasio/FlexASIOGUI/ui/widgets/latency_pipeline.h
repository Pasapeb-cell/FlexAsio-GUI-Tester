#pragma once

#include <QWidget>

#include "../../core/latency_calculator.h"

namespace flexasio_gui {

	// Paints a left-to-right pipeline diagram of the audio latency stages (ASIO buffer ->
	// PortAudio buffering -> Windows audio engine), each labeled with its millisecond
	// contribution, plus the total estimated one-way latency.
	class LatencyPipelineWidget final : public QWidget {
		Q_OBJECT
	public:
		explicit LatencyPipelineWidget(QWidget* parent = nullptr);
		void SetBreakdown(const LatencyBreakdown& breakdown, bool bypassesWindowsAudioEngine);
		QSize sizeHint() const override;

	protected:
		void paintEvent(QPaintEvent*) override;

	private:
		LatencyBreakdown breakdown;
		bool bypassesEngine = false;
	};

}
