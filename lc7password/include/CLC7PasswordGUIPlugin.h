#ifndef __INC_CLC7PASSWORDGUIPLUGIN_H
#define __INC_CLC7PASSWORDGUIPLUGIN_H

#include"CAccountsPage.h"
#include"CImportPage.h"
#include"CAuditPage.h"
#include"CStatisticsPage.h"
#include"CPasswordUIOptions.h"
#include"CLC7CalibrateGUI.h"
#include"CLC7ReportExportAccountsGUI.h"
#include"CLC7ReportExportStatisticsGUI.h"

class CLC7PasswordGUIPlugin:public QObject, public ILC7Plugin
{
	Q_OBJECT;

private:

	CAccountsPage *m_pAccountsPage;
	CImportPage *m_pImportPage;
	CAuditPage *m_pAuditPage;
	CStatisticsPage *m_pStatisticsPage;

	CLC7ReportExportAccountsGUI    *m_pReportExportAccountsGUI;
	CLC7ReportExportStatisticsGUI  *m_pReportExportStatisticsGUI;
	CLC7CalibrateGUI               *m_pCalibrateGUI;
	CPasswordUIOptions             *m_pPasswordUIOptions;

	ILC7ActionCategory *m_pSystemCat;
	ILC7Action         *m_pUISettingsAct;
	ILC7ActionCategory *m_pReportsCat;
	ILC7ActionCategory *m_pExportCat;
	ILC7Action         *m_pExportAccountsAct;
	ILC7Action         *m_pExportStatisticsAct;
	ILC7Action         *m_pCalibrateAct;

public:

	CLC7PasswordGUIPlugin();
	virtual ~CLC7PasswordGUIPlugin();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QUuid GetID();
	virtual QList<QUuid> GetInternalDependencies();

	virtual bool Activate();
	virtual bool Deactivate();


};

#endif