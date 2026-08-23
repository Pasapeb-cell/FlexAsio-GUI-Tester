#include "main_window.h"

#include <QIcon>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QTabWidget>
#include <QVBoxLayout>

#include "latency_tab.h"
#include "settings_tab.h"
#include "tester_tab.h"
#include "theme.h"

namespace flexasio_gui {

	namespace {

		// Title strip: wordmark, tagline, and a cyan-to-magenta rule underneath.
		class HeaderBanner final : public QWidget {
		public:
			explicit HeaderBanner(QWidget* parent = nullptr) : QWidget(parent) {
				setFixedHeight(74);
			}

		protected:
			void paintEvent(QPaintEvent*) override {
				QPainter painter(this);
				painter.setRenderHint(QPainter::Antialiasing);

				QLinearGradient background(0, 0, width(), 0);
				background.setColorAt(0.0, theme::kPanelHi);
				background.setColorAt(0.7, theme::kBackground);
				painter.fillRect(rect(), background);

				painter.setFont(theme::DisplayFont(22, /*bold=*/true, /*letterSpacingPercent=*/136));
				painter.setPen(theme::kText);
				painter.drawText(QRect(22, 12, width() - 44, 32), Qt::AlignVCenter | Qt::AlignLeft, "FLEXASIO");

				const int flexWidth = QFontMetrics(painter.font()).horizontalAdvance("FLEXASIO");
				painter.setPen(theme::kAccent);
				painter.drawText(QRect(22 + flexWidth + 10, 12, width(), 32), Qt::AlignVCenter | Qt::AlignLeft, "GUI TESTER");

				painter.setFont(theme::DisplayFont(8, /*bold=*/false, /*letterSpacingPercent=*/172));
				painter.setPen(theme::kTextDim);
				painter.drawText(QRect(24, 44, width() - 44, 18), Qt::AlignVCenter | Qt::AlignLeft,
					"BUFFER TUNING / LATENCY ANALYSIS / DROPOUT TESTING");

				QLinearGradient rule(0, 0, width(), 0);
				rule.setColorAt(0.0, theme::kAccent);
				rule.setColorAt(0.55, theme::kAccent2);
				QColor tail = theme::kAccent2; tail.setAlphaF(0.0f);
				rule.setColorAt(1.0, tail);
				painter.fillRect(QRect(0, height() - 2, width(), 2), rule);
			}
		};

	}

	MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
		setWindowTitle("FlexASIO GUI Tester");
		setWindowIcon(QIcon(":/icons/app_icon.ico"));
		resize(1000, 780);

		auto* root = new QWidget(this);
		auto* layout = new QVBoxLayout(root);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		layout->addWidget(new HeaderBanner());

		auto* tabs = new QTabWidget();
		tabs->setDocumentMode(true);
		auto* settingsTab = new SettingsTab();
		tabs->addTab(settingsTab, "Settings");
		tabs->addTab(new LatencyTab(*settingsTab), "Latency");
		tabs->addTab(new TesterTab(*settingsTab), "Tester");

		auto* tabsWrapper = new QWidget();
		auto* tabsLayout = new QVBoxLayout(tabsWrapper);
		tabsLayout->setContentsMargins(12, 12, 12, 12);
		tabsLayout->addWidget(tabs);
		layout->addWidget(tabsWrapper, 1);

		setCentralWidget(root);
	}

}
