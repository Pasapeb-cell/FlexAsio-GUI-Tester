#include <QApplication>
#include <QMessageBox>

#include <memory>
#include <string_view>
#include <windows.h>

#include "core/device_enumerator.h"
#include "ui/main_window.h"
#include "ui/theme.h"

int main(int argc, char** argv) {
	// Do this before loading Qt or audio backends. It prevents the current working
	// directory from participating in DLL resolution while retaining the app and
	// Windows system directories required by the portable build.
	if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)) return 2;
	const bool smokeTest = argc == 2 && std::string_view(argv[1]) == "--smoke-test";
	QApplication app(argc, argv);
	app.setProperty("flexasioGuiSmokeTest", smokeTest);
	app.setApplicationName("FlexASIO GUI Tester");
	flexasio_gui::theme::Apply(app);

	std::unique_ptr<flexasio_gui::PortAudioSession> portAudioSession;
	try {
		portAudioSession = std::make_unique<flexasio_gui::PortAudioSession>();
	}
	catch (const std::exception& exception) {
		if (smokeTest) return 1;
		QMessageBox::critical(nullptr, "FlexASIO GUI Tester",
			QString("Unable to initialize PortAudio: %1").arg(exception.what()));
		return 1;
	}

	flexasio_gui::MainWindow window;
	if (smokeTest) return 0;
	window.show();
	return app.exec();
}
