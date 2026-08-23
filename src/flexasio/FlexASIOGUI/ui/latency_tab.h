#pragma once

#include <QWidget>

class QTableWidget;

namespace flexasio_gui {

	class SettingsTab;
	class LatencyPipelineWidget;

	// Shows the estimated latency pipeline and a buffer-size comparison table, driven by
	// the current values in SettingsTab (backend, buffer size, sample rate, WASAPI mode).
	class LatencyTab final : public QWidget {
		Q_OBJECT
	public:
		explicit LatencyTab(SettingsTab& settingsTab, QWidget* parent = nullptr);

	public slots:
		void Refresh();

	private:
		SettingsTab& settingsTab;
		LatencyPipelineWidget* pipeline = nullptr;
		QTableWidget* comparisonTable = nullptr;
	};

}
