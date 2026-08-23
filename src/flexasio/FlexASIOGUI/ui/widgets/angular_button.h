#pragma once

#include <QAbstractButton>
#include <QColor>

namespace flexasio_gui {

	// Hexagonal button (flat top/bottom, angled ends) with a hover bloom, in the style of
	// arcade rhythm game menu entries.
	//
	// Primary emphasis is for the one dominant action on a screen; Danger is used for stop
	// / cancel so it reads distinctly from the cyan default.
	class AngularButton final : public QAbstractButton {
		Q_OBJECT
		Q_PROPERTY(qreal glow READ Glow WRITE SetGlow)
	public:
		enum class Emphasis { Normal, Primary, Danger };

		explicit AngularButton(const QString& text, QWidget* parent = nullptr);

		void SetEmphasis(Emphasis emphasis);
		QSize sizeHint() const override;
		QSize minimumSizeHint() const override { return sizeHint(); }

		qreal Glow() const { return glow; }
		void SetGlow(qreal value);

	protected:
		void paintEvent(QPaintEvent*) override;
		void enterEvent(QEnterEvent*) override;
		void leaveEvent(QEvent*) override;
		void changeEvent(QEvent*) override;

	private:
		void AnimateGlowTo(qreal target);
		QColor BaseAccent() const;

		Emphasis emphasis = Emphasis::Normal;
		qreal glow = 0.0;
	};

}
