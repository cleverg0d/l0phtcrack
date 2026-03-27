#include <stdafx.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include "CLC7ReportExportStatisticsGUI.h"

// ---------------------------------------------------------------------------
// Simple inline config widget — just a filename selector (CSV only)
// ---------------------------------------------------------------------------
class ExportStatisticsConfig : public QWidget
{
	Q_OBJECT

public:
	ExportStatisticsConfig(QWidget *parent, QWidget *page,
		const QString &defaultFilename)
		: QWidget(parent)
	{
		QVBoxLayout *vbox = new QVBoxLayout(this);
		vbox->setContentsMargins(0, 0, 0, 0);
		vbox->setSpacing(8);

		QLabel *desc = new QLabel(
			"Exports the Statistics page to a CSV file:\n"
			"  •  Crack Rate (all / active / disabled)\n"
			"  •  Password Complexity breakdown\n"
			"  •  Top Passwords table (transposed, as shown on screen)\n"
			"  •  Full password frequency list with accounts");
		desc->setWordWrap(true);
		vbox->addWidget(desc);

		vbox->addSpacing(8);
		vbox->addWidget(new QLabel("Output Filename:"));

		QHBoxLayout *hbox = new QHBoxLayout();
		m_filenameEdit = new QLineEdit();
		m_filenameEdit->setText(QDir::toNativeSeparators(defaultFilename));
		hbox->addWidget(m_filenameEdit, 1);

		QPushButton *browse = new QPushButton("Browse...");
		connect(browse, &QPushButton::clicked, this, &ExportStatisticsConfig::onBrowse);
		hbox->addWidget(browse);
		vbox->addLayout(hbox);

		vbox->addStretch();

		// Notify page of validity whenever filename changes
		connect(m_filenameEdit, &QLineEdit::textChanged,
			[page](const QString &text) {
				QMetaObject::invokeMethod(page, "slot_isValid",
					Q_ARG(bool, !text.trimmed().isEmpty()));
			});

		// Initial validity
		QMetaObject::invokeMethod(page, "slot_isValid",
			Q_ARG(bool, !defaultFilename.trimmed().isEmpty()));
	}

	QString filename() const { return m_filenameEdit->text().trimmed(); }

private slots:
	void onBrowse()
	{
		QString path = QFileDialog::getSaveFileName(
			this, "Export Statistics", m_filenameEdit->text(),
			"CSV Files (*.csv)");
		if (!path.isEmpty())
			m_filenameEdit->setText(QDir::toNativeSeparators(path));
	}

private:
	QLineEdit *m_filenameEdit;
};

#include "CLC7ReportExportStatisticsGUI.moc"

// ---------------------------------------------------------------------------

CLC7ReportExportStatisticsGUI::CLC7ReportExportStatisticsGUI() { TR; }
CLC7ReportExportStatisticsGUI::~CLC7ReportExportStatisticsGUI() { TR; }

ILC7Interface *CLC7ReportExportStatisticsGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component") return this;
	return nullptr;
}

QUuid CLC7ReportExportStatisticsGUI::GetID()
{
	return UUID_REPORTEXPORTSTATISTICSGUI;
}

ILC7Component::RETURNCODE CLC7ReportExportStatisticsGUI::ExecuteCommand(
	QString command, QStringList args,
	QMap<QString, QVariant> &config, QString &error,
	ILC7CommandControl * /*ctrl*/)
{
	TR;
	if (command == "gui" && args[0] == "create")
	{
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		ILC7Settings *settings = g_pLinkage->GetSettings();
		QString defFile = settings->value(
			UUID_REPORTEXPORTSTATISTICSGUI.toString() + ":default_filename",
			QString()).toString();

		ExportStatisticsConfig *widget = new ExportStatisticsConfig(page, page, defFile);
		config["widget"] = QVariant((qulonglong)widget);
		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "store")
	{
		ExportStatisticsConfig *widget =
			(ExportStatisticsConfig *)(config["widget"].toULongLong());

		QString filename = widget->filename();
		config["filename"]       = filename;
		config["display_string"] = filename;

		ILC7Settings *settings = g_pLinkage->GetSettings();
		settings->setValue(
			UUID_REPORTEXPORTSTATISTICSGUI.toString() + ":default_filename",
			filename);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "queue")
	{
		ILC7WorkQueue *pwq = (ILC7WorkQueue *)(config["workqueue"].toULongLong());
		LC7WorkQueueItem item(
			UUID_REPORTEXPORTSTATISTICS, "export", QStringList(), config,
			QString("Export Statistics (%1)").arg(config["display_string"].toString()),
			true, false);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}

	error = "Unknown command";
	return FAIL;
}

bool CLC7ReportExportStatisticsGUI::ValidateCommand(
	QMap<QString, QVariant> & /*state*/, QString /*command*/,
	QStringList /*args*/, QMap<QString, QVariant> & /*config*/, QString & /*error*/)
{
	return true;
}
