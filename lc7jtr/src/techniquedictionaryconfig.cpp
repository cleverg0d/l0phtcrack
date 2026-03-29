#include "stdafx.h"
#include "techniquedictionaryconfig.h"
#include <QFileDialog>

static QString findBundledWordlistsDir()
{
	QStringList candidates;
	QDir startup(g_pLinkage->GetStartupDirectory());
	candidates << startup.absoluteFilePath("wordlists");
	candidates << startup.absoluteFilePath("common/wordlists");
	candidates << startup.absoluteFilePath("../Resources/wordlists");
	candidates << startup.absoluteFilePath("../Resources/common/wordlists");
	candidates << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../../../dist/common/common/wordlists");
	foreach (QString candidate, candidates)
	{
		QDir d(candidate);
		if (d.exists())
		{
			return d.absolutePath();
		}
	}
	return startup.absoluteFilePath("wordlists");
}

static QString repairBundledWordlistPath(const QString &currentPath)
{
	if (currentPath.isEmpty() || currentPath.startsWith("$$FOLDER:") || QFileInfo(currentPath).exists())
	{
		return currentPath;
	}

	QDir bundled(findBundledWordlistsDir());
	const QString fileName = QFileInfo(currentPath).fileName();
	if (fileName.startsWith("wordlist-") && fileName.endsWith(".txt"))
	{
		const QString candidate = bundled.absoluteFilePath(fileName);
		if (QFileInfo(candidate).exists())
		{
			return QDir::toNativeSeparators(candidate);
		}
	}
	return currentPath;
}


TechniqueDictionaryConfig::TechniqueDictionaryConfig()
{TR;
	ui.setupUi(this);

	connect(ui.durationHoursText, &QLineEdit::textChanged, this, &TechniqueDictionaryConfig::slot_durationHoursText_textChanged);
	connect(ui.durationMinutesText, &QLineEdit::textChanged, this, &TechniqueDictionaryConfig::slot_durationMinutesText_textChanged);
	connect(ui.unlimitedCheckBox, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_unlimitedCheckBox_clicked);
	connect(ui.browseWordlistButton, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_browseWordlistButton_clicked);
	connect(ui.browseFolderButton, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_browseFolderButton_clicked);
	connect(ui.browseCustomRuleButton, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_browseCustomRuleButton_clicked);
	connect(ui.clearCustomRuleButton, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_clearCustomRuleButton_clicked);
	connect(ui.customRuleEdit, &QLineEdit::textChanged, this, &TechniqueDictionaryConfig::slot_customRuleEdit_textChanged);
	connect(ui.encodingCombo, (void (QComboBox::*)(int)) &QComboBox::currentIndexChanged, this, &TechniqueDictionaryConfig::slot_encodingCombo_currentIndexChanged);
	connect(ui.permutationRulesCombo, (void (QComboBox::*)(int)) &QComboBox::currentIndexChanged, this, &TechniqueDictionaryConfig::slot_permutationRulesCombo_currentIndexChanged);
	connect(ui.enableCommonLetterSubstitutions, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_enableCommonLetterSubstitutions_clicked);
	connect(ui.wordListEdit, &QLineEdit::textChanged, this, &TechniqueDictionaryConfig::slot_wordListEdit_textChanged);

	ui.durationHoursText->setValidator(new QIntValidator(0, 99999, this));
	ui.durationMinutesText->setValidator(new QIntValidator(0, 59, this));

	m_refreshing = false;

	RefreshContent();
}

TechniqueDictionaryConfig::~TechniqueDictionaryConfig()
{TR;
}


void TechniqueDictionaryConfig::setConfig(QVariant config)
{TR;
	m_config = config.toMap();

	if (m_config.isEmpty())
	{
		QDir wordlists(findBundledWordlistsDir());
		QString big = wordlists.absoluteFilePath("wordlist-big.txt");

		m_config["encoding"] = UUID_ENCODING_ISO_8859_1;
		m_config["rule"] = UUID_RULE_WORDLIST;
		m_config["wordlist"] = big;
		m_config["duration_unlimited"] = true;
		m_config["duration_hours"] = 1;
		m_config["duration_minutes"] = 0;
		m_config["custom_rule_file"] = QString();
	}
	else
	{
		m_config["wordlist"] = repairBundledWordlistPath(m_config["wordlist"].toString());
		if (!m_config.contains("custom_rule_file"))
			m_config["custom_rule_file"] = QString();
	}

	RefreshContent();
}

QVariant TechniqueDictionaryConfig::getConfig(void)
{TR;
	return m_config;
}


void TechniqueDictionaryConfig::slot_wordListEdit_textChanged(const QString & text)
{TR;
	if (m_refreshing) return;
	// Only update if not a folder (folder display text differs from stored value)
	if (!m_config["wordlist"].toString().startsWith("$$FOLDER:"))
	{
		m_config["wordlist"] = QDir::fromNativeSeparators(text);
	}
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_browseWordlistButton_clicked(bool checked)
{TR;
	QString startpath;
	QString currentWl = m_config["wordlist"].toString();
	if (currentWl.startsWith("$$FOLDER:"))
	{
		startpath = currentWl.mid(9);
	}
	else if (currentWl.isEmpty())
	{
		startpath = findBundledWordlistsDir();
	}
	else
	{
		startpath = QFileInfo(currentWl).dir().absolutePath();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose Wordlist File", startpath, "Wordlists (*.txt *.dic *.lst *.md);;All Files (*)", filepath))
	{
		m_config["wordlist"] = filepath;
		m_refreshing = true;
		ui.wordListEdit->setText(QDir::toNativeSeparators(filepath));
		m_refreshing = false;
	}

	UpdateUI();
}

void TechniqueDictionaryConfig::slot_browseFolderButton_clicked(bool checked)
{TR;
	QString currentWl = m_config["wordlist"].toString();
	QString startpath;
	if (currentWl.startsWith("$$FOLDER:"))
		startpath = currentWl.mid(9);
	else
		startpath = findBundledWordlistsDir();

	QString dirpath = QFileDialog::getExistingDirectory(this, "Choose Wordlist Folder", startpath,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!dirpath.isEmpty())
	{
		m_config["wordlist"] = QString("$$FOLDER:") + dirpath;
		m_refreshing = true;
		ui.wordListEdit->setText(QDir::toNativeSeparators(dirpath) + " [folder]");
		m_refreshing = false;
	}

	UpdateUI();
}

void TechniqueDictionaryConfig::slot_browseCustomRuleButton_clicked(bool checked)
{TR;
	QString currentRule = m_config["custom_rule_file"].toString();
	QString startpath = currentRule.isEmpty()
		? QString("/usr/local/share/doc/hashcat/rules")
		: QFileInfo(currentRule).dir().absolutePath();

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose Hashcat Rule File", startpath, "Rule Files (*.rule *.rules);;All Files (*)", filepath))
	{
		m_config["custom_rule_file"] = filepath;
		m_refreshing = true;
		ui.customRuleEdit->setText(QDir::toNativeSeparators(filepath));
		m_refreshing = false;
	}

	UpdateUI();
}

void TechniqueDictionaryConfig::slot_clearCustomRuleButton_clicked(bool checked)
{TR;
	m_config["custom_rule_file"] = QString();
	m_refreshing = true;
	ui.customRuleEdit->clear();
	m_refreshing = false;
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_customRuleEdit_textChanged(const QString & text)
{TR;
	if (m_refreshing) return;
	m_config["custom_rule_file"] = QDir::fromNativeSeparators(text);
	UpdateUI();
}


void TechniqueDictionaryConfig::slot_encodingCombo_currentIndexChanged(int index)
{TR;
	if (m_refreshing)
	{
		return;
	}

	QUuid encoding = ui.encodingCombo->itemData(index).toUuid();
	m_config["encoding"] = encoding;
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_permutationRulesCombo_currentIndexChanged(int index)
{TR;
	if (m_refreshing)
	{
		return;
	}

	QUuid rule = ui.permutationRulesCombo->itemData(index).toUuid();
	m_config["rule"] = rule;
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_durationHoursText_textChanged(const QString & text)
{TR;
	m_config["duration_hours"] = text.toInt();
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_durationMinutesText_textChanged(const QString & text)
{TR;
	m_config["duration_minutes"] = text.toInt();
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_unlimitedCheckBox_clicked(bool checked)
{TR;
	m_config["duration_unlimited"] = checked;
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_enableCommonLetterSubstitutions_clicked(bool checked)
{
	TR;
	m_config["leet"] = checked;
	UpdateUI();
}

void TechniqueDictionaryConfig::RefreshContent()
{TR;

	m_refreshing = true;

	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	// Display wordlist path - show folder nicely
	QString wl = m_config["wordlist"].toString();
	wl = repairBundledWordlistPath(wl);
	m_config["wordlist"] = wl;
	if (wl.startsWith("$$FOLDER:"))
	{
		ui.wordListEdit->setText(QDir::toNativeSeparators(wl.mid(9)) + " [folder]");
	}
	else
	{
		ui.wordListEdit->setText(QDir::toNativeSeparators(wl));
	}

	// Display custom rule file
	ui.customRuleEdit->setText(QDir::toNativeSeparators(m_config["custom_rule_file"].toString()));

	ui.unlimitedCheckBox->setChecked(m_config["duration_unlimited"].toBool());
	ui.durationHoursText->setText(QString("%1").arg(m_config["duration_hours"].toInt()));
	ui.durationMinutesText->setText(QString("%1").arg(m_config["duration_minutes"].toInt()));

	ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
	ILC7PresetGroup *encodings = manager->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
	if (encodings)
	{
		ui.encodingCombo->clear();

		QUuid defset = m_config["encoding"].toUuid();

		for (int i = 0; i < encodings->presetCount(); i++)
		{
			ILC7Preset *encoding = encodings->presetAt(i);
			ui.encodingCombo->addItem(encoding->name(), QVariant(encoding->id()));

			if (encoding->id() == defset)
			{
				ui.encodingCombo->setCurrentIndex(i);
			}
		}
	}

	ILC7PresetGroup *rules = manager->presetGroup(QString("%1:rules").arg(UUID_LC7JTRPLUGIN.toString()));
	if (rules)
	{
		ui.permutationRulesCombo->clear();

		QUuid defset = m_config["rule"].toUuid();

		for (int i = 0; i < rules->presetCount(); i++)
		{
			ILC7Preset *rule = rules->presetAt(i);
			ui.permutationRulesCombo->addItem(rule->name(), QVariant(rule->id()));

			if (rule->id() == defset)
			{
				ui.permutationRulesCombo->setCurrentIndex(i);
			}
		}
	}

	ui.enableCommonLetterSubstitutions->setChecked(m_config["leet"].toBool());

	UpdateUI();

	m_refreshing = false;
}

void TechniqueDictionaryConfig::UpdateUI()
{TR;
	ui.durationHoursText->setEnabled(!m_config["duration_unlimited"].toBool());
	ui.durationMinutesText->setEnabled(!m_config["duration_unlimited"].toBool());

	// Permutation rules combo is disabled when custom rule file is specified
	QString customRule = m_config["custom_rule_file"].toString();
	bool hasCustomRule = !customRule.isEmpty();
	ui.permutationRulesCombo->setEnabled(!hasCustomRule);
	ui.enableCommonLetterSubstitutions->setEnabled(!hasCustomRule);

	bool is_valid = true;
	QString why;
	if (m_config["encoding"].toUuid().isNull())
	{
		is_valid = false;
		why += "Encoding is not selected.\n";
	}
	if (!hasCustomRule && m_config["rule"].toUuid().isNull())
	{
		is_valid = false;
		why += "Rule is not selected.\n";
	}
	if (hasCustomRule && !QFileInfo(customRule).exists())
	{
		is_valid = false;
		why += "Custom rule file does not exist.\n";
	}

	// Validate wordlist
	QString wl = m_config["wordlist"].toString();
	bool wl_valid = false;
	if (wl.startsWith("$$FOLDER:"))
	{
		QDir dir(wl.mid(9));
		if (dir.exists())
		{
			QStringList found = dir.entryList({"*.txt","*.lst","*.dic","*.md"}, QDir::Files);
			wl_valid = !found.isEmpty();
			if (!wl_valid)
				why += "No wordlist files (*.txt/lst/dic/md) found in selected folder.\n";
		}
		else
		{
			why += "Selected folder does not exist.\n";
		}
	}
	else
	{
		wl_valid = !wl.isEmpty() && QFileInfo(wl).exists();
		if (!wl_valid)
			why += "Wordlist is not selected.\n";
	}
	if (!wl_valid) is_valid = false;

	if (!m_config["duration_unlimited"].toBool() &&
		((m_config["duration_hours"].toInt() == 0 && m_config["duration_minutes"].toInt() == 0) || m_config["duration_hours"].toInt() < 0 || m_config["duration_minutes"].toInt() < 0))
	{
		is_valid = false;
		why += "Invalid duration.\n";
	}

	emit sig_isValid(is_valid, why);
}
