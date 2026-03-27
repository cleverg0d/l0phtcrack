#ifndef __INC_CSTATISTICSPAGE_H
#define __INC_CSTATISTICSPAGE_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>

#include "lc7api.h"

class CLC7AccountList;

class CStatisticsPage : public QWidget
{
	Q_OBJECT

public slots:
	void slot_uiEnable(bool enable);
	void slot_accountsUpdated(ILC7AccountList::ACCOUNT_UPDATE_ACTION_LIST updates);

public:
	CStatisticsPage(QWidget *parent = nullptr);
	virtual ~CStatisticsPage();

	void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

protected:
	virtual void showEvent(QShowEvent *evt);

private:
	CLC7AccountList *m_accountlist;

	// Crack rate
	QLabel       *m_crackAllLabel;
	QProgressBar *m_crackAllBar;
	QLabel       *m_crackActiveLabel;
	QProgressBar *m_crackActiveBar;
	QLabel       *m_crackDisabledLabel;
	QProgressBar *m_crackDisabledBar;

	// Password complexity
	QTableWidget *m_complexityTable;

	// Top Passwords (merged transposed: columns = passwords, rows = users + Total)
	QTableWidget *m_topPwdTable;
	QLabel       *m_topPwdInfoLabel;

	void RefreshStatistics();
	void ClearStats();

	QWidget *BuildCrackRateSection();
	QWidget *BuildComplexitySection();
	QWidget *BuildTopPasswordsSection();
};

extern CStatisticsPage *CreateStatisticsPage();

#endif
