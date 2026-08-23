#pragma once

#include <QColor>
#include <QWidget>

class QVBoxLayout;

namespace flexasio_gui {

	// A chamfered (corner-cut) panel with a thin glowing border and an optional header tab
	// in the top-left, mirroring the section framing used in arcade rhythm game UIs.
	//
	// Children go into ContentLayout(), which is already inset to clear the header.
	class AngularPanel final : public QWidget {
		Q_OBJECT
	public:
		explicit AngularPanel(const QString& title = {}, QWidget* parent = nullptr);

		QVBoxLayout* ContentLayout() const { return contentLayout; }
		void SetAccent(const QColor& color);

	protected:
		void paintEvent(QPaintEvent*) override;

	private:
		QString title;
		QColor accent;
		QVBoxLayout* contentLayout = nullptr;
	};

}
