#include <stdafx.h>

CTechniqueJTRFinalyseGUI::CTechniqueJTRFinalyseGUI()
{
	TR;
}

CTechniqueJTRFinalyseGUI::~CTechniqueJTRFinalyseGUI()
{
	TR;
}

ILC7Interface *CTechniqueJTRFinalyseGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CTechniqueJTRFinalyseGUI::GetID()
{
	TR;
	return UUID_TECHNIQUEJTRFINALYSEGUI;
}

ILC7Component::RETURNCODE CTechniqueJTRFinalyseGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> &config, QString &error, ILC7CommandControl *ctrl)
{
	TR;
	if (command == "gui" && args.size() >= 1 && args[0] == "create")
	{
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());
		if (!page)
		{
			error = QStringLiteral("Audit UI error: missing page widget (internal).");
			return FAIL;
		}

		connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));
		emit sig_isValid(true);

		QWidget *widget = new QWidget();
		config["widget"] = QVariant((qulonglong)widget);
		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "store")
	{
		config.clear();
		config["name"] = QString("Finalyse");
		config["jtr_mode"] = "wordlist";
		config["wordlist"] = "$$CRACKED$$";
		config["encoding"] = UUID_ENCODING_ISO_8859_1;
		config["rule"] = UUID_RULE_BUKA_400K;
		config["leet"] = false;
		config["duration_unlimited"] = true;
		config["duration_hours"] = 1;
		config["duration_minutes"] = 0;
		config["display_string"] = config["name"].toString();
		return SUCCESS;
	}
	else if (command == "gui" && args.size() >= 1 && args[0] == "queue")
	{
		ILC7WorkQueue *pwq = (ILC7WorkQueue *)(config["workqueue"].toULongLong());
		config.remove("workqueue");
		LC7WorkQueueItem item(UUID_TECHNIQUEJTR, "crack", QStringList(), config,
			QString("Perform Finalyse Crack (%1)").arg(config["display_string"].toString()), true, true);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}

	return FAIL;
}

bool CTechniqueJTRFinalyseGUI::ValidateCommand(QMap<QString, QVariant> &state, QString command, QStringList args, QMap<QString, QVariant> &config, QString &error)
{
	TR;
	return true;
}
