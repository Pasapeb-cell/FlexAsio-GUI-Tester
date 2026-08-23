#include "theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QPalette>

namespace flexasio_gui::theme {

	namespace {
		QString Rgba(const QColor& c, qreal alpha = 1.0) {
			return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha, 0, 'f', 3);
		}

		QString PickFamily(const QStringList& candidates, const QString& fallback) {
			const auto families = QFontDatabase::families();
			for (const auto& candidate : candidates)
				if (families.contains(candidate, Qt::CaseInsensitive)) return candidate;
			return fallback;
		}
	}

	QPainterPath ChamferedRect(const QRectF& rect, qreal cut,
		bool topLeft, bool topRight, bool bottomRight, bool bottomLeft) {
		// Never let the cut exceed half the shorter side, or the polygon self-intersects.
		cut = qMin(cut, qMin(rect.width(), rect.height()) / 2.0);

		QPainterPath path;
		path.moveTo(rect.left() + (topLeft ? cut : 0), rect.top());
		path.lineTo(rect.right() - (topRight ? cut : 0), rect.top());
		if (topRight) path.lineTo(rect.right(), rect.top() + cut);
		path.lineTo(rect.right(), rect.bottom() - (bottomRight ? cut : 0));
		if (bottomRight) path.lineTo(rect.right() - cut, rect.bottom());
		path.lineTo(rect.left() + (bottomLeft ? cut : 0), rect.bottom());
		if (bottomLeft) path.lineTo(rect.left(), rect.bottom() - cut);
		path.lineTo(rect.left(), rect.top() + (topLeft ? cut : 0));
		path.closeSubpath();
		return path;
	}

	QPainterPath HexShape(const QRectF& rect, qreal slant) {
		slant = qMin(slant, rect.width() / 2.0);
		QPainterPath path;
		path.moveTo(rect.left() + slant, rect.top());
		path.lineTo(rect.right() - slant, rect.top());
		path.lineTo(rect.right(), rect.center().y());
		path.lineTo(rect.right() - slant, rect.bottom());
		path.lineTo(rect.left() + slant, rect.bottom());
		path.lineTo(rect.left(), rect.center().y());
		path.closeSubpath();
		return path;
	}

	QFont DisplayFont(int pointSize, bool bold, qreal letterSpacingPercent) {
		static const QString family = PickFamily({"Bahnschrift", "Eurostile", "Segoe UI Variable Display"}, "Segoe UI");
		QFont font(family, pointSize);
		font.setBold(bold);
		font.setLetterSpacing(QFont::PercentageSpacing, letterSpacingPercent);
		return font;
	}

	QFont MonoFont(int pointSize) {
		static const QString family = PickFamily({"Cascadia Mono", "Consolas"}, "Courier New");
		QFont font(family, pointSize);
		return font;
	}

	QString StyleSheet() {
		// Widgets that paint themselves (AngularPanel, AngularButton, the meters) are left
		// alone here; this covers the stock Qt controls.
		return QString(R"(
QMainWindow, QDialog, QMessageBox {
    background-color: %{bg};
}
QWidget#PanelContent, QWidget#TabPage {
    background: transparent;
}
QLabel {
    background: transparent;
    color: %{text};
}
QLabel[role="dim"] {
    color: %{textDim};
}

QTabWidget::pane {
    border: 1px solid %{accentDim};
    background-color: %{bgAlt};
    top: -1px;
}
QTabBar::tab {
    background-color: %{field};
    color: %{textDim};
    border: 1px solid %{accentDim};
    border-bottom: none;
    padding: 9px 30px;
    margin-right: 3px;
    font-weight: bold;
}
QTabBar::tab:hover {
    color: %{accent};
    background-color: %{panelHi};
}
QTabBar::tab:selected {
    color: %{accent};
    background-color: %{bgAlt};
    border-color: %{accent};
}

QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit {
    background-color: %{field};
    color: %{text};
    border: 1px solid %{accentDim};
    padding: 5px 9px;
    min-height: 20px;
    selection-background-color: %{accentSel};
}
QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover {
    border-color: %{accent};
}
QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
    border-color: %{accent};
    background-color: %{panelHi};
}
QComboBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {
    color: %{textDim};
    border-color: %{border};
}
QComboBox::drop-down {
    border: none;
    width: 22px;
}
QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid %{accent};
    width: 0; height: 0;
    margin-right: 8px;
}
QComboBox QAbstractItemView {
    background-color: %{bgAlt};
    color: %{text};
    border: 1px solid %{accent};
    selection-background-color: %{accentSel};
    selection-color: %{accent};
    outline: none;
}
QSpinBox::up-button, QDoubleSpinBox::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button {
    background-color: %{panelHi};
    border: 1px solid %{accentDim};
    width: 16px;
}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: %{accentSel};
}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
    image: none;
    border-left: 3px solid transparent;
    border-right: 3px solid transparent;
    border-bottom: 4px solid %{accent};
    width: 0; height: 0;
}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
    image: none;
    border-left: 3px solid transparent;
    border-right: 3px solid transparent;
    border-top: 4px solid %{accent};
    width: 0; height: 0;
}

QSlider::groove:horizontal {
    height: 4px;
    background-color: %{border};
}
QSlider::sub-page:horizontal {
    background-color: %{accent};
}
QSlider::handle:horizontal {
    width: 10px;
    height: 18px;
    margin: -8px -1px;
    background-color: %{accent};
    border: 1px solid %{bg};
}
QSlider::handle:horizontal:hover {
    background-color: %{accent2};
}

QCheckBox {
    background: transparent;
    color: %{text};
    spacing: 8px;
}
QCheckBox::indicator {
    width: 15px; height: 15px;
    background-color: %{field};
    border: 1px solid %{accentDim};
}
QCheckBox::indicator:hover {
    border-color: %{accent};
}
QCheckBox::indicator:checked {
    background-color: %{accent};
    border-color: %{accent};
}
QCheckBox:disabled {
    color: %{textDim};
}

QPushButton {
    background-color: %{field};
    color: %{accent};
    border: 1px solid %{accentDim};
    padding: 7px 18px;
    font-weight: bold;
}
QPushButton:hover {
    border-color: %{accent};
    background-color: %{panelHi};
}
QPushButton:pressed {
    background-color: %{accentSel};
}
QPushButton:disabled {
    color: %{textDim};
    border-color: %{border};
}

QTableWidget, QTableView {
    background-color: %{field};
    alternate-background-color: %{panel};
    color: %{text};
    gridline-color: %{border};
    border: 1px solid %{accentDim};
    outline: none;
}
QTableWidget::item:selected, QTableView::item:selected {
    background-color: %{accentSel};
    color: %{accent};
}
QHeaderView::section {
    background-color: %{panel};
    color: %{accent};
    border: none;
    border-bottom: 1px solid %{accent};
    border-right: 1px solid %{border};
    padding: 7px 6px;
    font-weight: bold;
}
QTableCornerButton::section {
    background-color: %{panel};
    border: none;
}

QScrollBar:vertical {
    background-color: %{field};
    width: 11px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background-color: %{accentDim};
    min-height: 26px;
}
QScrollBar::handle:vertical:hover {
    background-color: %{accent};
}
QScrollBar:horizontal {
    background-color: %{field};
    height: 11px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background-color: %{accentDim};
    min-width: 26px;
}
QScrollBar::handle:horizontal:hover {
    background-color: %{accent};
}
QScrollBar::add-line, QScrollBar::sub-line {
    width: 0; height: 0;
}
QScrollBar::add-page, QScrollBar::sub-page {
    background: transparent;
}

QToolTip {
    background-color: %{bgAlt};
    color: %{text};
    border: 1px solid %{accent};
    padding: 4px 7px;
}
)")
			.replace("%{bg}", kBackground.name())
			.replace("%{bgAlt}", kBackgroundAlt.name())
			.replace("%{panelHi}", kPanelHi.name())
			.replace("%{panel}", kPanel.name())
			.replace("%{field}", kField.name())
			.replace("%{accentDim}", kAccentDim.name())
			.replace("%{accentSel}", Rgba(kAccent, 0.22))
			.replace("%{accent2}", kAccent2.name())
			.replace("%{accent}", kAccent.name())
			.replace("%{textDim}", kTextDim.name())
			.replace("%{text}", kText.name())
			.replace("%{border}", QColor(0x1c, 0x2a, 0x36).name());
	}

	void Apply(QApplication& app) {
		app.setStyle("Fusion");

		QPalette palette;
		palette.setColor(QPalette::Window, kBackground);
		palette.setColor(QPalette::WindowText, kText);
		palette.setColor(QPalette::Base, kField);
		palette.setColor(QPalette::AlternateBase, kPanel);
		palette.setColor(QPalette::Text, kText);
		palette.setColor(QPalette::Button, kPanel);
		palette.setColor(QPalette::ButtonText, kAccent);
		palette.setColor(QPalette::Highlight, kAccent);
		palette.setColor(QPalette::HighlightedText, kBackground);
		palette.setColor(QPalette::ToolTipBase, kBackgroundAlt);
		palette.setColor(QPalette::ToolTipText, kText);
		palette.setColor(QPalette::Disabled, QPalette::Text, kTextDim);
		palette.setColor(QPalette::Disabled, QPalette::WindowText, kTextDim);
		palette.setColor(QPalette::Disabled, QPalette::ButtonText, kTextDim);
		app.setPalette(palette);

		app.setFont(DisplayFont(9, /*bold=*/false, /*letterSpacingPercent=*/104));
		app.setStyleSheet(StyleSheet());
	}

}
