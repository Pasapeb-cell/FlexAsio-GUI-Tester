#include "quality_indicator.h"

#include <QFont>
#include <QPainter>

namespace flexasio_gui {

	QualityIndicator::QualityIndicator(QWidget* parent) : QWidget(parent) {}

	void QualityIndicator::SetLevel(QualityLevel newLevel) {
		if (level == newLevel) return;
		level = newLevel;
		update();
	}

	QSize QualityIndicator::sizeHint() const { return QSize(220, 48); }

	void QualityIndicator::paintEvent(QPaintEvent*) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		QColor color;
		QString text;
		switch (level) {
		case QualityLevel::Green: color = QColor(0x2e, 0xa0, 0x43); text = "Stable"; break;
		case QualityLevel::Yellow: color = QColor(0xe0, 0xa8, 0x00); text = "Marginal"; break;
		case QualityLevel::Red: color = QColor(0xd0, 0x30, 0x30); text = "Unstable"; break;
		case QualityLevel::Unknown:
		default: color = QColor(0x80, 0x80, 0x80); text = "Not tested"; break;
		}

		const int diameter = qMin(height() - 8, 32);
		const QRect circleRect(4, (height() - diameter) / 2, diameter, diameter);
		painter.setPen(Qt::NoPen);
		painter.setBrush(color);
		painter.drawEllipse(circleRect);

		painter.setPen(palette().color(QPalette::WindowText));
		QFont font = painter.font();
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(QRect(circleRect.right() + 12, 0, width() - circleRect.right() - 16, height()),
			Qt::AlignVCenter | Qt::AlignLeft, text);
	}

}
