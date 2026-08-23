#include "auto_tuner.h"

#include <QTimer>

namespace flexasio_gui {

	namespace {
		const std::vector<int64_t> kCommonSizes = {
			32, 48, 64, 96, 128, 144, 160, 192, 240, 256,
			320, 384, 448, 480, 512, 768, 1024, 2048, 4096
		};
	}

	AutoTuner::AutoTuner(AudioEngine& engine, QObject* parent) : QObject(parent), engine(engine) {
		windowTimer = new QTimer(this);
		windowTimer->setSingleShot(true);
		connect(windowTimer, &QTimer::timeout, this, &AutoTuner::OnTestWindowElapsed);
	}

	void AutoTuner::Start(TestConfig baseConfig, int testDurationMs_) {
		config = std::move(baseConfig);
		testDurationMs = testDurationMs_;
		candidates = kCommonSizes;
		lo = 0;
		hi = int(candidates.size()) - 1;
		bestStableSize = candidates.back();
		TestNextCandidate();
	}

	void AutoTuner::Cancel() {
		windowTimer->stop();
		engine.Stop();
	}

	void AutoTuner::TestNextCandidate() {
		if (lo > hi) {
			engine.Stop();
			emit finished(qint64(bestStableSize));
			return;
		}

		const int mid = (lo + hi) / 2;
		config.bufferSizeSamples = candidates[size_t(mid)];
		emit progress(qint64(config.bufferSizeSamples), mid, int(candidates.size()));

		try {
			engine.Start(config);
		}
		catch (const std::exception&) {
			// This size failed to open at all (e.g. too small for the backend/device) -
			// treat it the same as "unstable" and search towards larger sizes.
			lo = mid + 1;
			TestNextCandidate();
			return;
		}
		windowTimer->start(testDurationMs);
	}

	void AutoTuner::OnTestWindowElapsed() {
		const bool stable = engine.TotalDropouts() == 0;
		engine.Stop();

		const int mid = (lo + hi) / 2;
		if (stable) {
			bestStableSize = candidates[size_t(mid)];
			hi = mid - 1;
		}
		else {
			lo = mid + 1;
		}
		TestNextCandidate();
	}

}
