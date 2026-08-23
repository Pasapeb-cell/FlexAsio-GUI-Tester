#pragma once

#include <QObject>
#include <QString>

#include <cstdint>
#include <vector>

#include "engine.h"

class QTimer;

namespace flexasio_gui {

	// Binary-searches through a list of common ASIO buffer sizes to find the smallest one
	// that produces zero output-underflow dropouts over a fixed test window. Drives the
	// given AudioEngine directly: for each candidate size it starts playback, waits
	// testDurationMs, then inspects the engine's dropout counter. A size that fails to open
	// at all (e.g. too small for the backend) is treated as unstable and the search moves to
	// larger sizes.
	class AutoTuner final : public QObject {
		Q_OBJECT
	public:
		explicit AutoTuner(AudioEngine& engine, QObject* parent = nullptr);

		void Start(TestConfig baseConfig, int testDurationMs = 5000);
		void Cancel();

	signals:
		void progress(qint64 bufferSizeSamples, int candidateIndex, int candidateCount);
		void finished(qint64 minimumStableBufferSize);
		void failed(QString reason);

	private slots:
		void OnTestWindowElapsed();

	private:
		void TestNextCandidate();

		AudioEngine& engine;
		TestConfig config;
		int testDurationMs = 5000;

		std::vector<int64_t> candidates;
		int lo = 0;
		int hi = 0;
		int64_t bestStableSize = 0;
		bool foundStableSize = false;
		QString lastOpenError;

		QTimer* windowTimer = nullptr;
	};

}
