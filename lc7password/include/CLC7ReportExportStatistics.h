#ifndef __INC_CLC7REPORTEXPORTSTATISTICS_H
#define __INC_CLC7REPORTEXPORTSTATISTICS_H

#include "../../lc7/include/ILC7GUIComponent.h"

class ILC7AccountList;

class CLC7ReportExportStatistics : public QObject, public ILC7Component
{
	Q_OBJECT;

private:
	ILC7AccountList *m_accountlist;

public:
	CLC7ReportExportStatistics();
	virtual ~CLC7ReportExportStatistics();

	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args,
		QMap<QString, QVariant> &config, QString &error, ILC7CommandControl *ctrl = nullptr);
	virtual bool ValidateCommand(QMap<QString, QVariant> &state, QString command,
		QStringList args, QMap<QString, QVariant> &config, QString &error);

	void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);
};

#endif
