#include <stdafx.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

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

		// Build the Finalyse options widget
		QWidget *widget = new QWidget();
		QVBoxLayout *mainLayout = new QVBoxLayout(widget);
		mainLayout->setContentsMargins(0, 0, 0, 0);

		QGroupBox *groupBox = new QGroupBox("Finalyse Options");
		QVBoxLayout *gbLayout = new QVBoxLayout(groupBox);

		QLabel *descLabel = new QLabel(
			"Applies permutation rules to all previously cracked passwords.\n"
			"Default: buka_400k.rule (400k hashcat-style mutations)."
		);
		descLabel->setWordWrap(true);
		gbLayout->addWidget(descLabel);

		m_allRulesCheckBox = new QCheckBox(
			"Use ALL hashcat rules (one sequential pass per .rule file)\n"
			"Directory: /usr/local/share/doc/hashcat/rules/"
		);
		gbLayout->addWidget(m_allRulesCheckBox);

		QLabel *noteLabel = new QLabel(
			"<small><i>Note: with all rules enabled, cracking takes longer but finds significantly more passwords.</i></small>"
		);
		noteLabel->setWordWrap(true);
		gbLayout->addWidget(noteLabel);

		groupBox->setLayout(gbLayout);
		mainLayout->addWidget(groupBox);
		mainLayout->addStretch();
		widget->setLayout(mainLayout);

		config["widget"] = QVariant((qulonglong)widget);
		return SUCCESS;
	}
	else if (command == "gui" && args.size() >= 1 && args[0] == "store")
	{
		bool allRules = m_allRulesCheckBox && m_allRulesCheckBox->isChecked();
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
		config["all_rules"] = allRules;
		config["custom_rule_file"] = QString();
		if (allRules)
			config["display_string"] = QString("Finalyse (All Rules)");
		else
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
