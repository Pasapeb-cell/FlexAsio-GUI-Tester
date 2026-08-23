#include "latency_pipeline.h"

#include <QFont>
#include <QPainter>

#include <vector>

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

	QSize LatencyPipelineWidget::sizeHint() const { return QSize(640, 160); }

	void LatencyPipelineWidget::paintEvent(QPaintEvent*) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		const std::vector<Stage> stages = {
			{"ASIO Buffer", breakdown.bufferLatencyMs, false},
			{"PortAudio Buffering", breakdown.suggestedLatencyMs, false},
			{bypassesEngine ? "Windows Audio Engine (bypassed)" : "Windows Audio Engine", breakdown.backendOverheadMs, bypassesEngine},
		};

		const int margin = 16;
		const int count = int(stages.size());
		const int gap = 24;
		const int boxWidth = (width() - margin * 2 - (count - 1) * gap) / count;
		const int boxHeight = 64;
		const int y = (height() - boxHeight) / 2 - 12;

		QFont labelFont = painter.font();
		labelFont.setBold(true);
		QFont msFont = painter.font();

		for (int i = 0; i < count; ++i) {
			const Stage& stage = stages[size_t(i)];
			const int x = margin + i * (boxWidth + gap);
			const QRect box(x, y, boxWidth, boxHeight);

			painter.setPen(Qt::NoPen);
			painter.setBrush(stage.muted ? QColor(0x3a, 0x3a, 0x3a) : QColor(0x2f, 0x6f, 0xb0));
			painter.drawRoundedRect(box, 8, 8);

			painter.setPen(Qt::white);
			painter.setFont(labelFont);
			painter.drawText(box.adjusted(6, 4, -6, -28), Qt::AlignCenter | Qt::TextWordWrap, stage.label);

			painter.setFont(msFont);
			painter.drawText(box.adjusted(6, boxHeight - 24, -6, -4), Qt::AlignCenter,
				QString::number(stage.ms, 'f', 2) + " ms");

			if (i < count - 1) {
				painter.setPen(QPen(palette().color(QPalette::WindowText), 2));
				const int arrowY = y + boxHeight / 2;
				painter.drawLine(x + boxWidth, arrowY, x + boxWidth + gap, arrowY);
			}
		}

		painter.setPen(palette().color(QPalette::WindowText));
		QFont totalFont = painter.font();
		totalFont.setBold(true);
		totalFont.setPointSize(totalFont.pointSize() + 2);
		painter.setFont(totalFont);
		painter.drawText(QRect(margin, y + boxHeight + 16, width() - margin * 2, 30), Qt::AlignLeft,
			QString("Total estimated one-way latency: %1 ms").arg(breakdown.totalLatencyMs, 0, 'f', 2));
	}

}
