#ifndef __INC_CLC7REPORTEXPORTSTATISTICSGUI_H
#define __INC_CLC7REPORTEXPORTSTATISTICSGUI_H

#include "../../lc7/include/ILC7GUIComponent.h"

class CLC7ReportExportStatisticsGUI : public QObject, public ILC7Component
{
	Q_OBJECT;

public:
	CLC7ReportExportStatisticsGUI();
	virtual ~CLC7ReportExportStatisticsGUI();

	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args,
		QMap<QString, QVariant> &config, QString &error, ILC7CommandControl *ctrl = nullptr);
	virtual bool ValidateCommand(QMap<QString, QVariant> &state, QString command,
		QStringList args, QMap<QString, QVariant> &config, QString &error);
};

#endif
