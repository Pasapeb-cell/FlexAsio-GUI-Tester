#include <QApplication>
#include <QMessageBox>

#include <memory>

#include "core/device_enumerator.h"
#include "ui/main_window.h"
#include "ui/theme.h"

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	app.setApplicationName("FlexASIO GUI Tester");
	flexasio_gui::theme::Apply(app);

	std::unique_ptr<flexasio_gui::PortAudioSession> portAudioSession;
	try {
		portAudioSession = std::make_unique<flexasio_gui::PortAudioSession>();
	}
	catch (const std::exception& exception) {
		QMessageBox::critical(nullptr, "FlexASIO GUI Tester",
			QString("Unable to initialize PortAudio: %1").arg(exception.what()));
		return 1;
	}

	flexasio_gui::MainWindow window;
	window.show();
	return app.exec();
}
