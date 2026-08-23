#include "latency_tab.h"

#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "../core/latency_calculator.h"
#include "settings_tab.h"
#include "theme.h"
#include "widgets/angular_panel.h"
#include "widgets/latency_pipeline.h"

namespace flexasio_gui {

	namespace {
		const std::vector<int64_t> kComparisonSizes = {32, 64, 128, 256, 512, 1024, 2048, 4096};
	}

	LatencyTab::LatencyTab(SettingsTab& settingsTab_, QWidget* parent) : QWidget(parent), settingsTab(settingsTab_) {
		setObjectName("TabPage");
		auto* layout = new QVBoxLayout(this);
		layout->setSpacing(12);

		auto* pipelinePanel = new AngularPanel("Signal Path");
		pipeline = new LatencyPipelineWidget();
		pipelinePanel->ContentLayout()->addWidget(pipeline);
		layout->addWidget(pipelinePanel);

		auto* tablePanel = new AngularPanel("Buffer Size Comparison");
		tablePanel->SetAccent(theme::kAccentWarm);
		comparisonTable = new QTableWidget();
		comparisonTable->setColumnCount(4);
		comparisonTable->setHorizontalHeaderLabels({"Size (samples)", "Buffer (ms)", "Est. Total (ms)", "Stability Risk"});
		comparisonTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		comparisonTable->verticalHeader()->setVisible(false);
		comparisonTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		comparisonTable->setSelectionMode(QAbstractItemView::NoSelection);
		comparisonTable->setShowGrid(false);
		comparisonTable->setAlternatingRowColors(true);
		tablePanel->ContentLayout()->addWidget(comparisonTable);
		layout->addWidget(tablePanel, 1);

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
			const auto risk = QString::fromStdString(StabilityRiskLabel(size, sampleRate));
			auto* riskItem = new QTableWidgetItem(risk);

			// Risk shades from magenta (very likely to crackle) through amber to green.
			QColor riskColor = theme::kOk;
			if (risk == "Very High" || risk == "High") riskColor = theme::kBad;
			else if (risk == "Medium") riskColor = theme::kWarn;
			riskItem->setForeground(riskColor);

			for (auto* item : {sizeItem, bufferItem, totalItem, riskItem})
				item->setTextAlignment(Qt::AlignCenter);

			if (size == bufferSize) {
				// Mark the row matching the configured buffer size.
				for (auto* item : {sizeItem, bufferItem, totalItem, riskItem}) {
					auto font = item->font();
					font.setBold(true);
					item->setFont(font);
					item->setBackground(QColor(theme::kAccent.red(), theme::kAccent.green(), theme::kAccent.blue(), 38));
				}
				sizeItem->setForeground(theme::kAccent);
			}

			comparisonTable->setItem(row, 0, sizeItem);
			comparisonTable->setItem(row, 1, bufferItem);
			comparisonTable->setItem(row, 2, totalItem);
			comparisonTable->setItem(row, 3, riskItem);
		}
	}

}
