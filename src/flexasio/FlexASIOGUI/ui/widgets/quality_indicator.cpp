#include "quality_indicator.h"

#include <QLinearGradient>
#include <QPainter>
#include <QRadialGradient>

#include "../theme.h"

namespace flexasio_gui {

	QualityIndicator::QualityIndicator(QWidget* parent) : QWidget(parent) {}

	void QualityIndicator::SetLevel(QualityLevel newLevel) {
		if (level == newLevel) return;
		level = newLevel;
		update();
	}

	QSize QualityIndicator::sizeHint() const { return QSize(300, 56); }

	void QualityIndicator::paintEvent(QPaintEvent*) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		QColor color;
		QString text;
		switch (level) {
		case QualityLevel::Green:  color = theme::kOk;   text = "Stable"; break;
		case QualityLevel::Yellow: color = theme::kWarn; text = "Marginal"; break;
		case QualityLevel::Red:    color = theme::kBad;  text = "Unstable"; break;
		case QualityLevel::Unknown:
		default:                   color = theme::kIdle; text = "Not tested"; break;
		}

		const QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
		const QPainterPath shape = theme::ChamferedRect(body, 12.0);

		// Body tinted toward the verdict colour so the whole strip reads at a glance.
		QLinearGradient fill(body.topLeft(), body.topRight());
		QColor tint = color; tint.setAlphaF(0.20f);
		QColor fade = color; fade.setAlphaF(0.02f);
		painter.fillPath(shape, QBrush(theme::kField));
		fill.setColorAt(0.0, tint);
		fill.setColorAt(1.0, fade);
		painter.fillPath(shape, fill);

		QColor edge = color; edge.setAlphaF(0.65f);
		painter.setPen(QPen(edge, 1));
		painter.drawPath(shape);

		// Lamp with a bloom halo.
		const qreal lampR = 11.0;
		const QPointF center(body.left() + 30, body.center().y());

		QRadialGradient halo(center, lampR * 2.6);
		QColor haloIn = color; haloIn.setAlphaF(0.55f);
		QColor haloOut = color; haloOut.setAlphaF(0.0f);
		halo.setColorAt(0.0, haloIn);
		halo.setColorAt(1.0, haloOut);
		painter.setPen(Qt::NoPen);
		painter.setBrush(halo);
		painter.drawEllipse(center, lampR * 2.6, lampR * 2.6);

		painter.setBrush(color);
		painter.drawEllipse(center, lampR, lampR);

		QColor hi = color.lighter(165);
		painter.setBrush(hi);
		painter.drawEllipse(center + QPointF(-lampR * 0.28, -lampR * 0.30), lampR * 0.34, lampR * 0.34);

		painter.setPen(color);
		painter.setFont(theme::DisplayFont(14, /*bold=*/true, /*letterSpacingPercent=*/152));
		painter.drawText(QRectF(center.x() + 30, body.top(), body.width() - center.x() - 22, body.height()),
			Qt::AlignVCenter | Qt::AlignLeft, text.toUpper());
	}

}
