#include <stdafx.h>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "CLC7ReportExportStatistics.h"
#include "CLC7AccountList.h"

CLC7ReportExportStatistics::CLC7ReportExportStatistics()
{
	TR;
	m_accountlist = nullptr;
	g_pLinkage->RegisterNotifySessionActivity(
		ACCOUNTLIST_HANDLER_ID, this,
		(void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))
		&CLC7ReportExportStatistics::NotifySessionActivity);
}

CLC7ReportExportStatistics::~CLC7ReportExportStatistics()
{
	TR;
	g_pLinkage->UnregisterNotifySessionActivity(
		ACCOUNTLIST_HANDLER_ID, this,
		(void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))
		&CLC7ReportExportStatistics::NotifySessionActivity);
}

ILC7Interface *CLC7ReportExportStatistics::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component") return this;
	return nullptr;
}

QUuid CLC7ReportExportStatistics::GetID()
{
	return UUID_REPORTEXPORTSTATISTICS;
}

void CLC7ReportExportStatistics::NotifySessionActivity(
	ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
			m_accountlist = (ILC7AccountList *)handler;
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
			m_accountlist = nullptr;
		break;
	default:
		break;
	}
}

ILC7Component::RETURNCODE CLC7ReportExportStatistics::ExecuteCommand(
	QString command, QStringList /*args*/,
	QMap<QString, QVariant> &config, QString &error,
	ILC7CommandControl *ctrl)
{
	TR;
	if (command != "export") return FAIL;

	QString filename = config.value("filename", "").toString();
	if (filename.isEmpty())
	{
		error = "No output filename specified.";
		return FAIL;
	}

	if (!m_accountlist)
	{
		error = "No session open.";
		return FAIL;
	}

	// ---- Compute statistics ----
	ILC7AccountList::STATS stats = m_accountlist->GetStats();
	int totalAll   = (int)stats.total;
	int crackedAll = (int)stats.cracked;
	int totalActive = 0, crackedActive = 0;
	int totalDisabled = 0, crackedDisabled = 0;

	QMap<QString, int>         pwdCount;
	QMap<QString, QStringList> pwdAccounts;

	int cplxEmpty = 0, cplxShort = 0, cplxMedium = 0, cplxLong = 0;
	int cplxUserMatch = 0, cplxNotCracked = 0;

	if (ctrl) ctrl->AppendToActivityLog("Computing statistics...\n");

	CLC7AccountList *acctList = (CLC7AccountList *)m_accountlist;
	acctList->Acquire();
	int cnt = acctList->GetAccountCount();

	for (int i = 0; i < cnt; i++)
	{
		const LC7Account *acct = acctList->GetAccountAtConstPtrFast(i);
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
			QString name = acct->domain.isEmpty()
				? acct->username
				: acct->domain + "\\" + acct->username;
			pwdAccounts[password].append(name);

			int len = password.length();
			if      (len == 0) cplxEmpty++;
			else if (len < 8)  cplxShort++;
			else if (len < 12) cplxMedium++;
			else               cplxLong++;

			if (!acct->username.isEmpty() &&
				password.toLower() == acct->username.toLower())
				cplxUserMatch++;
		}
		else cplxNotCracked++;
	}

	acctList->Release();

	// Sort passwords by frequency desc
	QList<QPair<int,QString>> sorted;
	for (auto it = pwdCount.constBegin(); it != pwdCount.constEnd(); ++it)
		sorted.append({it.value(), it.key()});
	std::sort(sorted.begin(), sorted.end(),
		[](const QPair<int,QString> &a, const QPair<int,QString> &b)
		{ return a.first > b.first; });

	// ---- Write CSV ----
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		error = QString("Cannot write to file: %1").arg(filename);
		return FAIL;
	}

	QTextStream out(&file);
	out.setCodec("UTF-8");

	auto pct = [](int c, int t) -> QString {
		return t > 0 ? QString("%1%").arg(100.0 * c / t, 0, 'f', 1) : "0.0%";
	};
	auto esc = [](const QString &s) -> QString {
		return "\"" + QString(s).replace("\"", "\"\"") + "\"";
	};

	out << "Statistics Export\n";
	out << "Generated:," << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n\n";

	// Section 1: Crack Rate
	out << "CRACK RATE\n";
	out << "," << esc("All accounts") << "," << esc("Active accounts") << "," << esc("Disabled accounts") << "\n";
	out << "Cracked," << crackedAll    << "," << crackedActive   << "," << crackedDisabled   << "\n";
	out << "Total,"   << totalAll      << "," << totalActive     << "," << totalDisabled     << "\n";
	out << "Percentage," << pct(crackedAll, totalAll) << "," << pct(crackedActive, totalActive) << "," << pct(crackedDisabled, totalDisabled) << "\n\n";

	// Section 2: Password Complexity
	out << "PASSWORD COMPLEXITY\n";
	out << "Category,Count\n";
	out << "Empty (0 chars),"     << cplxEmpty      << "\n";
	out << "Short (1-7 chars),"   << cplxShort      << "\n";
	out << "Medium (8-11 chars)," << cplxMedium     << "\n";
	out << "Long (12+ chars),"    << cplxLong       << "\n";
	out << "Username = Password," << cplxUserMatch  << "\n";
	out << "Not cracked,"         << cplxNotCracked << "\n\n";

	// Section 3: Top Passwords (transposed — mirrors the Statistics screen)
	int pwdCols  = qMin(10, sorted.size());
	int userRows = 0;
	for (int c = 0; c < pwdCols; c++)
		userRows = qMax(userRows, (int)pwdAccounts[sorted[c].second].size());

	out << "TOP PASSWORDS";
	if (sorted.size() > 10)
		out << " (top 10 of " << sorted.size() << " unique)";
	out << "\n";

	// Header row: "#", Pwd1, Pwd2, ...
	out << "#";
	for (int c = 0; c < pwdCols; c++)
	{
		QString pw = sorted[c].second;
		out << "," << esc(pw.isEmpty() ? "<empty>" : pw);
	}
	out << "\n";

	// User rows
	for (int r = 0; r < userRows; r++)
	{
		out << (r + 1);
		for (int c = 0; c < pwdCols; c++)
		{
			const QStringList &users = pwdAccounts[sorted[c].second];
			out << ",";
			if (r < users.size())
				out << esc(users[r]);
		}
		out << "\n";
	}

	// Total row: "Total", "14 (5.5%)", ...
	out << "Total";
	for (int c = 0; c < pwdCols; c++)
	{
		int    n   = sorted[c].first;
		double pct2 = (crackedAll > 0) ? (100.0 * n / crackedAll) : 0.0;
		out << "," << esc(QString("%1 (%2%)").arg(n).arg(pct2, 0, 'f', 1));
	}
	out << "\n\n";

	// Section 4: All passwords (full list, not capped at 10)
	out << "ALL PASSWORDS (full list, " << sorted.size() << " unique)\n";
	out << "Rank,Password,Count,Percentage,Accounts\n";
	for (int i = 0; i < sorted.size(); i++)
	{
		int     n   = sorted[i].first;
		QString pw  = sorted[i].second;
		double  p   = (crackedAll > 0) ? (100.0 * n / crackedAll) : 0.0;
		const QStringList &users = pwdAccounts[pw];
		out << (i + 1) << "," << esc(pw.isEmpty() ? "<empty>" : pw) << ","
		    << n << "," << QString("%1%").arg(p, 0, 'f', 1) << ","
		    << esc(users.join("; ")) << "\n";
	}

	file.close();

	if (ctrl)
		ctrl->AppendToActivityLog(QString("Statistics exported to: %1\n").arg(filename));

	return SUCCESS;
}

bool CLC7ReportExportStatistics::ValidateCommand(
	QMap<QString, QVariant> & /*state*/, QString /*command*/,
	QStringList /*args*/, QMap<QString, QVariant> & /*config*/, QString & /*error*/)
{
	return true;
}
