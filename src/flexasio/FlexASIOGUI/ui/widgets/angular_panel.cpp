#include "angular_panel.h"

#include <QLinearGradient>
#include <QPainter>
#include <QVBoxLayout>

#include "../theme.h"

namespace flexasio_gui {

	namespace {
		constexpr qreal kCut = 14.0;
		constexpr int kHeaderHeight = 24;
	}

	AngularPanel::AngularPanel(const QString& title_, QWidget* parent)
		: QWidget(parent), title(title_.toUpper()), accent(theme::kAccent) {
		auto* outer = new QVBoxLayout(this);
		const int topInset = title.isEmpty() ? 14 : kHeaderHeight + 12;
		outer->setContentsMargins(14, topInset, 14, 14);

		auto* content = new QWidget(this);
		content->setObjectName("PanelContent");
		outer->addWidget(content);

		contentLayout = new QVBoxLayout(content);
		contentLayout->setContentsMargins(0, 0, 0, 0);
		contentLayout->setSpacing(8);
	}

	void AngularPanel::SetAccent(const QColor& color) {
		accent = color;
		update();
	}

	void AngularPanel::paintEvent(QPaintEvent*) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		const QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
		const QPainterPath shape = theme::ChamferedRect(body, kCut);

		QLinearGradient fill(body.topLeft(), body.bottomLeft());
		fill.setColorAt(0.0, theme::kPanelHi);
		fill.setColorAt(1.0, theme::kPanel);
		painter.fillPath(shape, fill);

		// Border, plus a soft inner bloom so the edge reads as lit rather than drawn.
		QColor glow = accent;
		glow.setAlphaF(0.18f);
		painter.setPen(QPen(glow, 3));
		painter.drawPath(shape);

		QColor edge = accent;
		edge.setAlphaF(0.75f);
		painter.setPen(QPen(edge, 1));
		painter.drawPath(shape);

		if (title.isEmpty()) return;

		// Header tab: a chamfered strip whose width tracks the title text.
		QFont font = theme::DisplayFont(8, /*bold=*/true, /*letterSpacingPercent=*/150);
		painter.setFont(font);
		const int textWidth = QFontMetrics(font).horizontalAdvance(title);
		const QRectF tab(body.left() + 1, body.top() + 1, textWidth + 34.0, kHeaderHeight);

		const QPainterPath tabShape = theme::ChamferedRect(
			tab, 12.0, /*topLeft=*/true, /*topRight=*/false, /*bottomRight=*/true, /*bottomLeft=*/false);

		QLinearGradient tabFill(tab.topLeft(), tab.topRight());
		QColor c0 = accent; c0.setAlphaF(0.85f);
		QColor c1 = accent; c1.setAlphaF(0.12f);
		tabFill.setColorAt(0.0, c0);
		tabFill.setColorAt(1.0, c1);
		painter.fillPath(tabShape, tabFill);

		painter.setPen(theme::kBackground);
		painter.drawText(tab.adjusted(16, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, title);

		// Hairline running from the tab to the right edge, tying the header to the frame.
		QColor rule = accent;
		rule.setAlphaF(0.30f);
		painter.setPen(QPen(rule, 1));
		const qreal ruleY = tab.bottom() + 0.5;
		painter.drawLine(QPointF(tab.right() + 6, ruleY), QPointF(body.right() - 8, ruleY));
	}

}
