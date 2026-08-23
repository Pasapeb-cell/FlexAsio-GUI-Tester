#include "latency_tab.h"

#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "../core/latency_calculator.h"
#include "settings_tab.h"
#include "widgets/latency_pipeline.h"

namespace flexasio_gui {

	namespace {
		const std::vector<int64_t> kComparisonSizes = {32, 64, 128, 256, 512, 1024, 2048, 4096};
	}

	LatencyTab::LatencyTab(SettingsTab& settingsTab_, QWidget* parent) : QWidget(parent), settingsTab(settingsTab_) {
		auto* layout = new QVBoxLayout(this);

		pipeline = new LatencyPipelineWidget();
		layout->addWidget(pipeline);

		comparisonTable = new QTableWidget();
		comparisonTable->setColumnCount(4);
		comparisonTable->setHorizontalHeaderLabels({"Size (samples)", "Buffer (ms)", "Est. Total (ms)", "Stability Risk"});
		comparisonTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		comparisonTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		comparisonTable->setSelectionMode(QAbstractItemView::NoSelection);
		layout->addWidget(comparisonTable, 1);

		connect(&settingsTab, &SettingsTab::configChanged, this, &LatencyTab::Refresh);
		Refresh();
	}

	void LatencyTab::Refresh() {
		const auto backendName = settingsTab.CurrentConfig().backend.value_or("Windows DirectSound");
		const double sampleRate = settingsTab.SampleRateHint();
		const int64_t bufferSize = settingsTab.BufferSizeSamples();
		const bool wasapiExclusive = settingsTab.WasapiExclusive(/*input=*/false);

		const auto config = settingsTab.CurrentConfig();
		const auto breakdown = ComputeLatency(bufferSize, sampleRate, config.output.suggestedLatencySeconds, backendName, wasapiExclusive);
		const bool bypassesEngine = (backendName == "Windows WDM-KS") || (backendName == "Windows WASAPI" && wasapiExclusive);
		pipeline->SetBreakdown(breakdown, bypassesEngine);

		comparisonTable->setRowCount(int(kComparisonSizes.size()));
		for (int row = 0; row < int(kComparisonSizes.size()); ++row) {
			const int64_t size = kComparisonSizes[size_t(row)];
			const auto rowBreakdown = ComputeLatency(size, sampleRate, config.output.suggestedLatencySeconds, backendName, wasapiExclusive);

			auto* sizeItem = new QTableWidgetItem(QString::number(size));
			auto* bufferItem = new QTableWidgetItem(QString::number(rowBreakdown.bufferLatencyMs, 'f', 2));
			auto* totalItem = new QTableWidgetItem(QString::number(rowBreakdown.totalLatencyMs, 'f', 2));
			auto* riskItem = new QTableWidgetItem(QString::fromStdString(StabilityRiskLabel(size, sampleRate)));

			if (size == bufferSize) {
				for (auto* item : {sizeItem, bufferItem, totalItem, riskItem})
					item->setFont([&] { auto font = item->font(); font.setBold(true); return font; }());
			}

			comparisonTable->setItem(row, 0, sizeItem);
			comparisonTable->setItem(row, 1, bufferItem);
			comparisonTable->setItem(row, 2, totalItem);
			comparisonTable->setItem(row, 3, riskItem);
		}
	}

}
