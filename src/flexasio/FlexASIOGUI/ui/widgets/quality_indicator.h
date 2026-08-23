#pragma once

#include <QWidget>

namespace flexasio_gui {

	enum class QualityLevel { Unknown, Green, Yellow, Red };

	// Traffic-light indicator: a colored circle plus a label describing audio stability
	// (Stable / Marginal / Unstable) based on the current dropout rate.
	class QualityIndicator final : public QWidget {
		Q_OBJECT
	public:
		explicit QualityIndicator(QWidget* parent = nullptr);
		void SetLevel(QualityLevel level);
		QSize sizeHint() const override;

	protected:
		void paintEvent(QPaintEvent*) override;

	private:
		QualityLevel level = QualityLevel::Unknown;
	};

}
