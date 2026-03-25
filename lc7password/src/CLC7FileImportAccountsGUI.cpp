#include<stdafx.h>

CLC7FileImportAccountsGUI::CLC7FileImportAccountsGUI()
{TR;
}

CLC7FileImportAccountsGUI::~CLC7FileImportAccountsGUI()
{TR;
}

ILC7Interface *CLC7FileImportAccountsGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CLC7FileImportAccountsGUI::GetID()
{TR;
	return UUID_FILEIMPORTACCOUNTSGUI;
}

ILC7Component::RETURNCODE CLC7FileImportAccountsGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if (command == "gui" && args[0] == "create")
	{
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		QWidget *widget = new QWidget(page);
		QVBoxLayout *layout = new QVBoxLayout(widget);

		QLabel *label = new QLabel(QObject::tr("Filename:"), widget);
		layout->addWidget(label);

		QLineEdit *filenameEdit = new QLineEdit(widget);
		filenameEdit->setObjectName("filenameEdit");
		filenameEdit->setText(config["filename"].toString());
		layout->addWidget(filenameEdit);

		QPushButton *browseButton = new QPushButton(QObject::tr("Browse..."), widget);
		layout->addWidget(browseButton);

		QObject::connect(browseButton, &QPushButton::clicked, [filenameEdit]() {
			QString fname = QFileDialog::getOpenFileName(nullptr,
				QObject::tr("Select hash file"),
				QString(),
				QObject::tr("Hash Files (*.txt *.pwdump *.dump *.ntds *.dit);;All Files (*)"));
			if (!fname.isEmpty())
			{
				filenameEdit->setText(fname);
			}
		});

		layout->addStretch();
		widget->setLayout(layout);

		config["widget"] = QVariant((qulonglong)widget);
		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "store")
	{
		QWidget *widget = (QWidget *)(config["widget"].toULongLong());
		QLineEdit *filenameEdit = widget->findChild<QLineEdit *>("filenameEdit");

		QString filename;
		if (filenameEdit)
		{
			filename = filenameEdit->text();
		}

		config["filename"] = filename;
		config["keep_current_accounts"] = false;
		config["display_string"] = QString("Import from file: %1").arg(filename);

		ILC7Settings *settings = g_pLinkage->GetSettings();
		QMap<QString, QVariant> def_config = config;
		def_config.remove("widget");
		settings->setValue(UUID_FILEIMPORTACCOUNTSGUI.toString() + ":defaults", def_config);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "queue")
	{
		ILC7WorkQueue *pwq = (ILC7WorkQueue *)(config["workqueue"].toULongLong());
		LC7WorkQueueItem item(UUID_FILEIMPORTACCOUNTS, "import", QStringList(), config,
			QString("Import hashes from file (%1)").arg(config["display_string"].toString()), true, false);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}

	return FAIL;
}

bool CLC7FileImportAccountsGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}
