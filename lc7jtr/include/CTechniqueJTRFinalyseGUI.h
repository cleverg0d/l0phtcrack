#ifndef __INC_CTECHNIQUEJTRFINALYSEGUI_H
#define __INC_CTECHNIQUEJTRFINALYSEGUI_H

#include "lc7api.h"

class CTechniqueJTRFinalyseGUI : public QObject, public ILC7Component
{
	Q_OBJECT

public:
	CTechniqueJTRFinalyseGUI();
	virtual ~CTechniqueJTRFinalyseGUI();

	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);
	virtual QUuid GetID();
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> &config, QString &error, ILC7CommandControl *ctrl);
	virtual bool ValidateCommand(QMap<QString, QVariant> &state, QString command, QStringList args, QMap<QString, QVariant> &config, QString &error);

signals:
	void sig_isValid(bool valid);
};

#endif
