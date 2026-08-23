#include "main_window.h"

#include <QTabWidget>

#include "latency_tab.h"
#include "settings_tab.h"
#include "tester_tab.h"

namespace flexasio_gui {

	MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
		setWindowTitle("FlexASIO GUI Tester");
		resize(960, 720);

		auto* tabs = new QTabWidget();
		auto* settingsTab = new SettingsTab();
		tabs->addTab(settingsTab, "Settings");
		tabs->addTab(new LatencyTab(*settingsTab), "Latency");
		tabs->addTab(new TesterTab(*settingsTab), "Tester");
		setCentralWidget(tabs);
	}

}
