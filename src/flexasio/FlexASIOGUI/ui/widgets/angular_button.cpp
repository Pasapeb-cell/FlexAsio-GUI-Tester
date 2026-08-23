#include "angular_button.h"

#include <QEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPropertyAnimation>

#include "../theme.h"

namespace flexasio_gui {

	namespace {
		constexpr qreal kSlant = 11.0;
		constexpr int kPaddingH = 26;
		constexpr int kHeightNormal = 34;
		constexpr int kHeightPrimary = 46;

		QFont ButtonFont(AngularButton::Emphasis emphasis) {
			return emphasis == AngularButton::Emphasis::Primary
				? theme::DisplayFont(13, true, 140)
				: theme::DisplayFont(9, true, 128);
		}
	}

	AngularButton::AngularButton(const QString& text, QWidget* parent)
		: QAbstractButton(parent) {
		setText(text.toUpper());
		setCursor(Qt::PointingHandCursor);
		setFocusPolicy(Qt::StrongFocus);
	}

	void AngularButton::SetEmphasis(Emphasis newEmphasis) {
		emphasis = newEmphasis;
		updateGeometry();
		update();
	}

	QColor AngularButton::BaseAccent() const {
		switch (emphasis) {
		case Emphasis::Danger: return theme::kAccent2;
		case Emphasis::Primary:
		case Emphasis::Normal:
		default: return theme::kAccent;
		}
	}

	QSize AngularButton::sizeHint() const {
		const QFontMetrics metrics(ButtonFont(emphasis));
		const int height = emphasis == Emphasis::Primary ? kHeightPrimary : kHeightNormal;
		return QSize(metrics.horizontalAdvance(text()) + kPaddingH * 2 + int(kSlant * 2), height);
	}

	void AngularButton::SetGlow(qreal value) {
		glow = value;
		update();
	}

	void AngularButton::AnimateGlowTo(qreal target) {
		auto* animation = new QPropertyAnimation(this, "glow", this);
		animation->setDuration(140);
		animation->setStartValue(glow);
		animation->setEndValue(target);
		animation->start(QAbstractAnimation::DeleteWhenStopped);
	}

	void AngularButton::enterEvent(QEnterEvent* event) {
		QAbstractButton::enterEvent(event);
		if (isEnabled()) AnimateGlowTo(1.0);
	}

	void AngularButton::leaveEvent(QEvent* event) {
		QAbstractButton::leaveEvent(event);
		AnimateGlowTo(0.0);
	}

	void AngularButton::changeEvent(QEvent* event) {
		QAbstractButton::changeEvent(event);
		// Dropping out of the hover state while disabled would otherwise strand the glow on.
		if (event->type() == QEvent::EnabledChange && !isEnabled()) {
			glow = 0.0;
			update();
		}
	}

	void AngularButton::paintEvent(QPaintEvent*) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		const QRectF body = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
		const QPainterPath shape = theme::HexShape(body, kSlant);
		const bool enabled = isEnabled();
		const bool pressed = isDown();

		const QColor accent = BaseAccent();

		if (!enabled) {
			painter.fillPath(shape, QColor(theme::kField));
			painter.setPen(QPen(QColor(0x1c, 0x2a, 0x36), 1));
			painter.drawPath(shape);
			painter.setPen(theme::kTextDim);
			painter.setFont(ButtonFont(emphasis));
			painter.drawText(body, Qt::AlignCenter, text());
			return;
		}

		// Body fill: brighter for the primary action, and brighter still while pressed.
		// Alphas are float because QColor::setAlphaF takes float in Qt 6.
		QLinearGradient fill(body.topLeft(), body.bottomLeft());
		const float lift = float((emphasis == Emphasis::Primary ? 0.26 : 0.10)
			+ glow * 0.20 + (pressed ? 0.16 : 0.0));
		QColor top = accent; top.setAlphaF(qMin(1.0f, lift));
		QColor bottom = accent; bottom.setAlphaF(qMin(1.0f, lift * 0.35f));
		fill.setColorAt(0.0, top);
		fill.setColorAt(1.0, bottom);
		painter.fillPath(QPainterPath(shape), QBrush(theme::kField));
		painter.fillPath(shape, fill);

		// Outer bloom, strongest on hover.
		if (glow > 0.01) {
			QColor halo = accent;
			halo.setAlphaF(float(0.30 * glow));
			painter.setPen(QPen(halo, 5));
			painter.drawPath(shape);
		}

		QColor edge = accent;
		edge.setAlphaF(emphasis == Emphasis::Primary ? 1.0f : float(0.55 + 0.45 * glow));
		painter.setPen(QPen(edge, emphasis == Emphasis::Primary ? 2 : 1));
		painter.drawPath(shape);

		// Primary buttons get a magenta counter-accent on the leading edge.
		if (emphasis == Emphasis::Primary) {
			painter.setPen(QPen(theme::kAccent2, 2));
			painter.drawLine(QPointF(body.left() + kSlant * 0.35, body.center().y() + kSlant * 0.6),
				QPointF(body.left() + kSlant, body.bottom()));
		}

		painter.setFont(ButtonFont(emphasis));
		painter.setPen(emphasis == Emphasis::Primary ? theme::kText : accent.lighter(glow > 0.5 ? 118 : 100));
		painter.drawText(body, Qt::AlignCenter, text());

		if (hasFocus()) {
			QColor focusRing = accent;
			focusRing.setAlphaF(0.5f);
			painter.setPen(QPen(focusRing, 1, Qt::DotLine));
			painter.drawPath(theme::HexShape(body.adjusted(4, 4, -4, -4), kSlant * 0.7));
		}
	}

}
