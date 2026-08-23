#include "level_meter.h"

#include <QPainter>

#include <algorithm>

#include "../theme.h"

namespace flexasio_gui {

	LevelMeter::LevelMeter(QWidget* parent) : QWidget(parent) {
		setMinimumHeight(42);
	}

	void LevelMeter::SetPeaks(float left, float right) {
		leftPeak = std::clamp(left, 0.0f, 1.0f);
		rightPeak = std::clamp(right, 0.0f, 1.0f);
		update();
	}

	QSize LevelMeter::sizeHint() const { return QSize(300, 42); }

	void LevelMeter::paintEvent(QPaintEvent*) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		const int labelWidth = 20;
		const int gap = 5;
		const int barHeight = 14;
		const int barWidth = width() - labelWidth - 4;
		const auto drawBar = [&](int y, float value, const QString& label) {
			painter.setFont(theme::MonoFont(8));
			painter.setPen(theme::kTextDim);
			painter.drawText(QRect(0, y, labelWidth, barHeight), Qt::AlignLeft | Qt::AlignVCenter, label);

			const QRectF bar(labelWidth, y + 1, barWidth, barHeight - 2);
			painter.fillPath(theme::ChamferedRect(bar, 3), theme::kField);
			QColor edge = theme::kAccentDim;
			painter.setPen(QPen(edge, 1));
			painter.drawPath(theme::ChamferedRect(bar, 3));

			const QRectF fill(bar.left(), bar.top(), bar.width() * value, bar.height());
			if (fill.width() > 0.0) {
				const QColor color = value > 0.90f ? theme::kBad : value > 0.72f ? theme::kWarn : theme::kOk;
				painter.fillPath(theme::ChamferedRect(fill, qMin<qreal>(3, fill.width() / 2)), color);
			}
		};

		drawBar(2, leftPeak, "L");
		drawBar(2 + barHeight + gap, rightPeak, "R");
	}

}
