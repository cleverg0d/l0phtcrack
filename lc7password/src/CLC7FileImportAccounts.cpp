#include<stdafx.h>

CLC7FileImportAccounts::CLC7FileImportAccounts()
{TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7FileImportAccounts::NotifySessionActivity);
	m_accountlist = NULL;
}

CLC7FileImportAccounts::~CLC7FileImportAccounts()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7FileImportAccounts::NotifySessionActivity);
}

ILC7Interface *CLC7FileImportAccounts::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CLC7FileImportAccounts::GetID()
{TR;
	return UUID_FILEIMPORTACCOUNTS;
}

void CLC7FileImportAccounts::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = (ILC7AccountList *)handler;
		}
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = NULL;
		}
		break;
	default:
		break;
	}
}

ILC7Component::RETURNCODE CLC7FileImportAccounts::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if (command == "import")
	{
		if (!m_accountlist)
		{
			error = "No account list available.";
			return FAIL;
		}

		if (config.contains("keep_current_accounts") && !config["keep_current_accounts"].toBool())
		{
			m_accountlist->ClearAccounts();
		}

		QString filename = config["filename"].toString();
		if (filename.isEmpty())
		{
			error = "No filename specified.";
			return FAIL;
		}

		bool cancelled = false;
		CAccountsImport importer(m_accountlist, ctrl);
		importer.setFilename(filename);
		if (!importer.DoImport(error, cancelled))
		{
			return FAIL;
		}
		if (cancelled)
		{
			return STOPPED;
		}

		return SUCCESS;
	}

	return FAIL;
}

bool CLC7FileImportAccounts::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	if (command == "import")
	{
		QString filename = config["filename"].toString();
		if (filename.isEmpty())
		{
			error = "No filename specified.";
			return false;
		}
		if (!QFileInfo(filename).isFile())
		{
			error = QString("File not found: %1").arg(filename);
			return false;
		}

		state["cracked"] = false;
		return true;
	}

	return true;
}
