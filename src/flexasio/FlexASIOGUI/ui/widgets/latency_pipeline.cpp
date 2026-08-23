#include "latency_pipeline.h"

#include <QLinearGradient>
#include <QPainter>

#include <vector>

#include "../theme.h"

namespace flexasio_gui {

	namespace {
		struct Stage {
			QString label;
			double ms;
			bool muted;
		};
	}

	LatencyPipelineWidget::LatencyPipelineWidget(QWidget* parent) : QWidget(parent) {}

	void LatencyPipelineWidget::SetBreakdown(const LatencyBreakdown& newBreakdown, bool newBypassesEngine) {
		breakdown = newBreakdown;
		bypassesEngine = newBypassesEngine;
		update();
	}

	QSize LatencyPipelineWidget::sizeHint() const { return QSize(640, 190); }

	void LatencyPipelineWidget::paintEvent(QPaintEvent*) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		const std::vector<Stage> stages = {
			{"ASIO Buffer", breakdown.bufferLatencyMs, false},
			{"PortAudio", breakdown.suggestedLatencyMs, false},
			{bypassesEngine ? "Win Audio (bypassed)" : "Windows Audio", breakdown.backendOverheadMs, bypassesEngine},
		};

		const int margin = 10;
		const int count = int(stages.size());
		const int gap = 30;
		const int boxHeight = 76;
		const int boxWidth = (width() - margin * 2 - (count - 1) * gap) / count;
		const int top = 14;

		const QFont labelFont = theme::DisplayFont(9, /*bold=*/true, /*letterSpacingPercent=*/126);
		const QFont msFont = theme::DisplayFont(15, /*bold=*/true, /*letterSpacingPercent=*/106);

		for (int i = 0; i < count; ++i) {
			const Stage& stage = stages[size_t(i)];
			const qreal x = margin + i * (boxWidth + gap);
			const QRectF box(x, top, boxWidth, boxHeight);
			const QPainterPath shape = theme::ChamferedRect(box, 13.0);

			const QColor accent = stage.muted ? theme::kIdle : theme::kAccent;

			painter.fillPath(shape, QBrush(theme::kField));
			QLinearGradient fill(box.topLeft(), box.bottomLeft());
			QColor c0 = accent; c0.setAlphaF(stage.muted ? 0.08f : 0.26f);
			QColor c1 = accent; c1.setAlphaF(0.03f);
			fill.setColorAt(0.0, c0);
			fill.setColorAt(1.0, c1);
			painter.fillPath(shape, fill);

			QColor glow = accent; glow.setAlphaF(0.16f);
			painter.setPen(QPen(glow, 3));
			painter.drawPath(shape);
			QColor edge = accent; edge.setAlphaF(stage.muted ? 0.4f : 0.8f);
			painter.setPen(QPen(edge, 1));
			painter.drawPath(shape);

			painter.setFont(labelFont);
			painter.setPen(stage.muted ? theme::kTextDim : theme::kAccent);
			painter.drawText(box.adjusted(10, 8, -10, -boxHeight / 2), Qt::AlignHCenter | Qt::AlignTop,
				stage.label.toUpper());

			painter.setFont(msFont);
			painter.setPen(stage.muted ? theme::kTextDim : theme::kText);
			painter.drawText(box.adjusted(10, boxHeight / 2 - 6, -10, -8), Qt::AlignHCenter | Qt::AlignTop,
				QString::number(stage.ms, 'f', 2) + " ms");

			if (i < count - 1) {
				const qreal arrowY = box.center().y();
				const qreal x0 = box.right() + 7;
				const qreal x1 = box.right() + gap - 7;
				QColor line = theme::kAccent; line.setAlphaF(0.5f);
				painter.setPen(QPen(line, 1.5));
				painter.drawLine(QPointF(x0, arrowY), QPointF(x1 - 5, arrowY));
				QPainterPath head;
				head.moveTo(x1, arrowY);
				head.lineTo(x1 - 6, arrowY - 4.5);
				head.lineTo(x1 - 6, arrowY + 4.5);
				head.closeSubpath();
				painter.fillPath(head, theme::kAccent);
			}
		}

		// Total, with a magenta rule to separate it from the stage row.
		const qreal totalY = top + boxHeight + 20;
		painter.setPen(QPen(theme::kAccent2, 2));
		painter.drawLine(QPointF(margin, totalY), QPointF(margin + 46, totalY));

		painter.setFont(theme::DisplayFont(8, /*bold=*/true, /*letterSpacingPercent=*/168));
		painter.setPen(theme::kTextDim);
		painter.drawText(QRectF(margin + 58, totalY - 10, width(), 20), Qt::AlignVCenter | Qt::AlignLeft,
			"TOTAL ESTIMATED ONE-WAY");

		painter.setFont(theme::DisplayFont(20, /*bold=*/true, /*letterSpacingPercent=*/104));
		painter.setPen(theme::kAccent);
		painter.drawText(QRectF(margin, totalY + 12, width() - margin * 2, 34), Qt::AlignLeft | Qt::AlignVCenter,
			QString::number(breakdown.totalLatencyMs, 'f', 2) + " ms");
	}

}
