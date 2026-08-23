#pragma once

#include <QMainWindow>

namespace flexasio_gui {

	// Top-level window: Settings, Latency, and Tester tabs sharing one SettingsTab instance
	// as the source of truth for backend/device/buffer-size selection.
	class MainWindow final : public QMainWindow {
		Q_OBJECT
	public:
		explicit MainWindow(QWidget* parent = nullptr);
	};

}
