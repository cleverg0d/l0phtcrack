#include <stdafx.h>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QShowEvent>

#include "CStatisticsPage.h"
#include "CLC7AccountList.h"

// ---------------------------------------------------------------------------
static void addCrackRateRow(QGridLayout *grid, int row,
	const QString &title, QLabel **outLabel, QProgressBar **outBar)
{
	QLabel *titleLabel = new QLabel(title);
	titleLabel->setMinimumWidth(110);

	QProgressBar *bar = new QProgressBar();
	bar->setRange(0, 100);
	bar->setValue(0);
	bar->setTextVisible(false);
	bar->setFixedHeight(16);

	QLabel *countLabel = new QLabel("0 / 0  (0.0%)");
	countLabel->setMinimumWidth(150);

	grid->addWidget(titleLabel, row, 0);
	grid->addWidget(bar,        row, 1);
	grid->addWidget(countLabel, row, 2);

	if (outBar)   *outBar   = bar;
	if (outLabel) *outLabel = countLabel;
}

static void setCrackRateRow(QLabel *label, QProgressBar *bar, int cracked, int total)
{
	if (!label || !bar) return;
	double pct = (total > 0) ? (100.0 * cracked / total) : 0.0;
	label->setText(QString("%1 / %2  (%3%)")
		.arg(cracked).arg(total).arg(pct, 0, 'f', 1));
	bar->setValue(qRound(pct));
}

// ---------------------------------------------------------------------------
CStatisticsPage::CStatisticsPage(QWidget *parent)
	: QWidget(parent)
	, m_accountlist(nullptr)
	, m_crackAllLabel(nullptr),    m_crackAllBar(nullptr)
	, m_crackActiveLabel(nullptr), m_crackActiveBar(nullptr)
	, m_crackDisabledLabel(nullptr), m_crackDisabledBar(nullptr)
	, m_complexityTable(nullptr)
	, m_topPwdTable(nullptr), m_topPwdInfoLabel(nullptr)
{
	TR;

	QScrollArea *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);

	QWidget *content = new QWidget();
	QVBoxLayout *vbox = new QVBoxLayout(content);
	vbox->setContentsMargins(12, 12, 12, 12);
	vbox->setSpacing(10);

	// Row 1: Crack Rate | Password Complexity (50/50)
	QHBoxLayout *topRow = new QHBoxLayout();
	topRow->setSpacing(10);
	topRow->addWidget(BuildCrackRateSection(), 1);
	topRow->addWidget(BuildComplexitySection(), 1);
	vbox->addLayout(topRow);

	// Row 2: Top Passwords (full width)
	vbox->addWidget(BuildTopPasswordsSection(), 1);

	content->setLayout(vbox);
	scroll->setWidget(content);

	QVBoxLayout *outerLayout = new QVBoxLayout(this);
	outerLayout->setContentsMargins(0, 0, 0, 0);
	outerLayout->addWidget(scroll);
	setLayout(outerLayout);

	g_pLinkage->RegisterNotifySessionActivity(
		ACCOUNTLIST_HANDLER_ID, this,
		(void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))
		&CStatisticsPage::NotifySessionActivity);

	ClearStats();
}

CStatisticsPage::~CStatisticsPage()
{
	TR;
	g_pLinkage->UnregisterNotifySessionActivity(
		ACCOUNTLIST_HANDLER_ID, this,
		(void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))
		&CStatisticsPage::NotifySessionActivity);
}

// ---------------------------------------------------------------------------
QWidget *CStatisticsPage::BuildCrackRateSection()
{
	QGroupBox *gb = new QGroupBox("Crack Rate");
	QGridLayout *grid = new QGridLayout(gb);
	grid->setColumnStretch(1, 1);
	grid->setHorizontalSpacing(10);
	grid->setVerticalSpacing(6);

	addCrackRateRow(grid, 0, "All accounts:",      &m_crackAllLabel,      &m_crackAllBar);
	addCrackRateRow(grid, 1, "Active accounts:",   &m_crackActiveLabel,   &m_crackActiveBar);
	addCrackRateRow(grid, 2, "Disabled accounts:", &m_crackDisabledLabel, &m_crackDisabledBar);

	gb->setLayout(grid);
	return gb;
}

QWidget *CStatisticsPage::BuildComplexitySection()
{
	QGroupBox *gb = new QGroupBox("Password Complexity");
	QVBoxLayout *vbox = new QVBoxLayout(gb);

	m_complexityTable = new QTableWidget(0, 2);
	m_complexityTable->setHorizontalHeaderLabels({"Category", "Count"});
	m_complexityTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_complexityTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_complexityTable->verticalHeader()->setVisible(false);
	m_complexityTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_complexityTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_complexityTable->setAlternatingRowColors(false);

	vbox->addWidget(m_complexityTable, 1);
	gb->setLayout(vbox);
	return gb;
}

QWidget *CStatisticsPage::BuildTopPasswordsSection()
{
	QGroupBox *gb = new QGroupBox("Top Passwords");
	QVBoxLayout *vbox = new QVBoxLayout(gb);
	vbox->setSpacing(4);

	// Columns built dynamically in RefreshStatistics
	m_topPwdTable = new QTableWidget(0, 1);
	m_topPwdTable->verticalHeader()->setVisible(false);
	m_topPwdTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_topPwdTable->setSelectionBehavior(QAbstractItemView::SelectColumns);
	m_topPwdTable->setAlternatingRowColors(false);
	m_topPwdTable->setMinimumHeight(200);

	m_topPwdInfoLabel = new QLabel();
	m_topPwdInfoLabel->setStyleSheet("color: gray; font-size: 11px;");

	vbox->addWidget(m_topPwdTable, 1);
	vbox->addWidget(m_topPwdInfoLabel);
	gb->setLayout(vbox);
	return gb;
}

// ---------------------------------------------------------------------------
void CStatisticsPage::ClearStats()
{
	setCrackRateRow(m_crackAllLabel,      m_crackAllBar,      0, 0);
	setCrackRateRow(m_crackActiveLabel,   m_crackActiveBar,   0, 0);
	setCrackRateRow(m_crackDisabledLabel, m_crackDisabledBar, 0, 0);

	if (m_complexityTable) m_complexityTable->setRowCount(0);

	if (m_topPwdTable)
	{
		m_topPwdTable->clear();
		m_topPwdTable->setColumnCount(0);
		m_topPwdTable->setRowCount(0);
	}
	if (m_topPwdInfoLabel) m_topPwdInfoLabel->clear();
}

// ---------------------------------------------------------------------------
void CStatisticsPage::RefreshStatistics()
{
	if (!m_accountlist) { ClearStats(); return; }

	ILC7AccountList::STATS stats = m_accountlist->GetStats();
	int totalAll   = (int)stats.total;
	int crackedAll = (int)stats.cracked;

	int totalActive = 0, crackedActive = 0;
	int totalDisabled = 0, crackedDisabled = 0;

	QMap<QString, int>         pwdCount;
	QMap<QString, QStringList> pwdAccounts;

	int cplxEmpty = 0, cplxShort = 0, cplxMedium = 0, cplxLong = 0;
	int cplxUserMatch = 0, cplxNotCracked = 0;

	m_accountlist->Acquire();
	int count = m_accountlist->GetAccountCount();

	for (int i = 0; i < count; i++)
	{
		const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(i);
		if (!acct) continue;

		bool isDisabled = (acct->disabled != 0);
		bool isCracked  = false;
		QString password;

		for (const LC7Hash &h : acct->hashes)
		{
			if (h.crackstate == CRACKSTATE_CRACKED)
			{
				isCracked = true;
				password  = h.password;
				break;
			}
		}

		if (isDisabled) { totalDisabled++; if (isCracked) crackedDisabled++; }
		else            { totalActive++;   if (isCracked) crackedActive++;   }

		if (isCracked)
		{
			pwdCount[password]++;
			QString acctName = acct->domain.isEmpty()
				? acct->username
				: acct->domain + "\\" + acct->username;
			pwdAccounts[password].append(acctName);

			int len = password.length();
			if      (len == 0) cplxEmpty++;
			else if (len < 8)  cplxShort++;
			else if (len < 12) cplxMedium++;
			else               cplxLong++;

			if (!acct->username.isEmpty() &&
				password.toLower() == acct->username.toLower())
				cplxUserMatch++;
		}
		else
		{
			cplxNotCracked++;
		}
	}

	m_accountlist->Release();

	// ---- Crack rate bars ----
	setCrackRateRow(m_crackAllLabel,      m_crackAllBar,      crackedAll,     totalAll);
	setCrackRateRow(m_crackActiveLabel,   m_crackActiveBar,   crackedActive,  totalActive);
	setCrackRateRow(m_crackDisabledLabel, m_crackDisabledBar, crackedDisabled, totalDisabled);

	// ---- Password complexity ----
	struct Row { QString label; int value; };
	const QList<Row> cplxRows = {
		{ "Empty (0 chars)",     cplxEmpty      },
		{ "Short (1-7 chars)",   cplxShort      },
		{ "Medium (8-11 chars)", cplxMedium     },
		{ "Long (12+ chars)",    cplxLong       },
		{ "Username = Password", cplxUserMatch  },
		{ "Not cracked",         cplxNotCracked },
	};
	m_complexityTable->setRowCount(0);
	for (int i = 0; i < cplxRows.size(); i++)
	{
		m_complexityTable->insertRow(i);
		m_complexityTable->setItem(i, 0, new QTableWidgetItem(cplxRows[i].label));
		auto *ci = new QTableWidgetItem(QString::number(cplxRows[i].value));
		ci->setTextAlignment(Qt::AlignCenter);
		m_complexityTable->setItem(i, 1, ci);
	}

	// ---- Top Passwords (transposed, full width) ----
	// Sort all passwords by frequency desc
	QList<QPair<int,QString>> sorted;
	for (auto it = pwdCount.constBegin(); it != pwdCount.constEnd(); ++it)
		sorted.append({it.value(), it.key()});
	std::sort(sorted.begin(), sorted.end(),
		[](const QPair<int,QString> &a, const QPair<int,QString> &b)
		{ return a.first > b.first; });

	int pwdCols  = qMin(10, sorted.size());
	int userRows = 0;
	for (int c = 0; c < pwdCols; c++)
		userRows = qMax(userRows, qMin(10, (int)pwdAccounts[sorted[c].second].size()));

	m_topPwdTable->clear();
	m_topPwdTable->setColumnCount(1 + pwdCols);  // "#" + N passwords
	m_topPwdTable->setRowCount(userRows);          // user rows only (no separate Total row)

	// Column headers two-line: password on top, "N · X.X%" below
	QStringList headers;
	headers << "#";
	for (int c = 0; c < pwdCols; c++)
	{
		QString pw  = sorted[c].second;
		int     cnt = sorted[c].first;
		double  pct = (crackedAll > 0) ? (100.0 * cnt / crackedAll) : 0.0;
		QString label = QString("%1\n%2 · %3%")
			.arg(pw.isEmpty() ? "<empty>" : pw)
			.arg(cnt)
			.arg(pct, 0, 'f', 1);
		headers << label;
	}
	m_topPwdTable->setHorizontalHeaderLabels(headers);

	// Smaller font for the header so two lines fit without too much height
	QFont hdrFont = m_topPwdTable->horizontalHeader()->font();
	hdrFont.setPointSize(qMax(7, hdrFont.pointSize() - 2));
	hdrFont.setBold(true);
	m_topPwdTable->horizontalHeader()->setFont(hdrFont);
	m_topPwdTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
	m_topPwdTable->horizontalHeader()->setMinimumSectionSize(60);

	m_topPwdTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	for (int c = 1; c <= pwdCols; c++)
		m_topPwdTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);

	// User rows
	for (int r = 0; r < userRows; r++)
	{
		auto *rowNum = new QTableWidgetItem(QString::number(r + 1));
		rowNum->setTextAlignment(Qt::AlignCenter);
		m_topPwdTable->setItem(r, 0, rowNum);

		for (int c = 0; c < pwdCols; c++)
		{
			const QStringList &users = pwdAccounts[sorted[c].second];
			QString cellText;
			if (r < users.size())
			{
				if (r == 9 && users.size() > 10)
					cellText = QString("+%1 more").arg(users.size() - 9);
				else
					cellText = users[r];
			}
			m_topPwdTable->setItem(r, c + 1, new QTableWidgetItem(cellText));
		}
	}

	// Info label
	int totalUnique = sorted.size();
	m_topPwdInfoLabel->setText(
		totalUnique > 10
		? QString("Top 10 of %1 unique passwords").arg(totalUnique)
		: QString("%1 unique password%2").arg(totalUnique).arg(totalUnique == 1 ? "" : "s"));
}

// ---------------------------------------------------------------------------
void CStatisticsPage::slot_uiEnable(bool /*enable*/) { TR; }

void CStatisticsPage::slot_accountsUpdated(ILC7AccountList::ACCOUNT_UPDATE_ACTION_LIST /*updates*/)
{
	TR;
	RefreshStatistics();
}

void CStatisticsPage::NotifySessionActivity(
	ILC7Linkage::SESSION_ACTIVITY activity,
	ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = (CLC7AccountList *)handler;
			m_accountlist->RegisterUpdateNotification(
				this,
				(void (QObject::*)(ILC7AccountList::ACCOUNT_UPDATE_ACTION_LIST))
				&CStatisticsPage::slot_accountsUpdated);
			RefreshStatistics();
		}
		break;

	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = nullptr;
			ClearStats();
		}
		break;
	}
}

void CStatisticsPage::showEvent(QShowEvent *evt)
{
	QWidget::showEvent(evt);
	if (m_accountlist)
		RefreshStatistics();
}

CStatisticsPage *CreateStatisticsPage()
{
	return new CStatisticsPage();
}
