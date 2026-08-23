#pragma once

#include <QColor>
#include <QFont>
#include <QPainterPath>
#include <QRectF>
#include <QString>

class QApplication;

// Arcade-rhythm-game-inspired visual language: near-black ground, cyan primary with a
// magenta counter-accent, chamfered (corner-cut) panels, hexagonal buttons, and wide
// letter-spaced uppercase type.
//
// Only the visual vocabulary is borrowed - no third-party logos, wordmarks, or assets.
namespace flexasio_gui::theme {

	// --- Palette ---------------------------------------------------------------------
	inline const QColor kBackground     {0x08, 0x0b, 0x11};
	inline const QColor kBackgroundAlt  {0x0d, 0x14, 0x1c};
	inline const QColor kPanel          {0x10, 0x18, 0x22};
	inline const QColor kPanelHi        {0x16, 0x21, 0x2d};
	inline const QColor kField          {0x0b, 0x11, 0x18};

	inline const QColor kAccent         {0x22, 0xd3, 0xee}; // cyan
	inline const QColor kAccentDim      {0x1a, 0x5a, 0x6b};
	inline const QColor kAccent2        {0xff, 0x2d, 0x78}; // magenta
	inline const QColor kAccentWarm     {0xff, 0x8c, 0x1a}; // orange, for section tabs

	inline const QColor kText           {0xd8, 0xe9, 0xf2};
	inline const QColor kTextDim        {0x74, 0x8c, 0x9e};

	inline const QColor kOk             {0x3d, 0xdc, 0x84};
	inline const QColor kWarn           {0xff, 0xb0, 0x20};
	inline const QColor kBad            {0xff, 0x2d, 0x55};
	inline const QColor kIdle           {0x42, 0x58, 0x6a};

	// --- Geometry --------------------------------------------------------------------

	// Rectangle with corners cut at 45 degrees. Cutting only the top-left and
	// bottom-right is the house style for panels; buttons cut all four.
	QPainterPath ChamferedRect(const QRectF& rect, qreal cut,
		bool topLeft = true, bool topRight = false,
		bool bottomRight = true, bool bottomLeft = false);

	// Flat top and bottom with angled left/right ends, for buttons.
	QPainterPath HexShape(const QRectF& rect, qreal slant);

	// --- Type ------------------------------------------------------------------------

	// Wide-tracked uppercase display face. Prefers Bahnschrift (ships with Windows 10+,
	// condensed and technical) and falls back to Segoe UI.
	QFont DisplayFont(int pointSize, bool bold = true, qreal letterSpacingPercent = 118);
	QFont MonoFont(int pointSize);

	// --- Application -----------------------------------------------------------------
	QString StyleSheet();
	void Apply(QApplication& app);

}
