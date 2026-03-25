#include <stdafx.h>

#include "lc7diagnosticlog.h"

#include <cstdio>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRecursiveMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

static QRecursiveMutex g_lc7_diag_mutex;

static void lc7_diag_stderr(const char *utf8_line)
{
	if (!utf8_line)
	{
		return;
	}
	std::fprintf(stderr, "[lc7-diag] %s\n", utf8_line);
	std::fflush(stderr);
}

static QStringList lc7_diag_candidate_bases(void)
{
	QStringList out;
	const QString appLocal = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	if (!appLocal.isEmpty())
	{
		out.append(appLocal);
	}
	out.append(QDir::homePath() + QStringLiteral("/.lc7"));
	const QString temp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	if (!temp.isEmpty())
	{
		out.append(temp);
	}
	out.append(QStringLiteral("/tmp"));
	return out;
}

static QString g_lc7_diag_resolved_path;

static QString lc7_diag_pick_writable_file_unlocked(void)
{
	if (!g_lc7_diag_resolved_path.isEmpty())
	{
		QFile probe(g_lc7_diag_resolved_path);
		if (probe.open(QIODevice::Append | QIODevice::Text))
		{
			probe.close();
			return g_lc7_diag_resolved_path;
		}
		g_lc7_diag_resolved_path.clear();
	}
	for (const QString &base : lc7_diag_candidate_bases())
	{
		if (base.isEmpty())
		{
			continue;
		}
		QDir().mkpath(base);
		const QString path =
			QDir(base).absoluteFilePath(QStringLiteral("lc7-diagnostic.log"));
		QFile wf(path);
		if (wf.open(QIODevice::Append | QIODevice::Text))
		{
			wf.close();
			g_lc7_diag_resolved_path = path;
			return g_lc7_diag_resolved_path;
		}
	}
	return QString();
}

QString LC7DiagnosticLogResolvedPath(void)
{
	QMutexLocker lock(&g_lc7_diag_mutex);
	return lc7_diag_pick_writable_file_unlocked();
}

extern "C" {

DLLEXPORT void LC7DiagnosticLog(const char *utf8_line)
{
	if (!utf8_line || !utf8_line[0])
	{
		return;
	}

	QMutexLocker lock(&g_lc7_diag_mutex);

	const QString path = lc7_diag_pick_writable_file_unlocked();
	if (path.isEmpty())
	{
		lc7_diag_stderr(utf8_line);
		return;
	}

	QFile f(path);
	if (!f.open(QIODevice::Append | QIODevice::Text))
	{
		g_lc7_diag_resolved_path.clear();
		lc7_diag_stderr(utf8_line);
		return;
	}
	QTextStream out(&f);
	const QString ts = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
	out << ts << QLatin1Char(' ') << QString::fromUtf8(utf8_line) << QLatin1Char('\n');
	f.flush();
}

DLLEXPORT void LC7DiagnosticLogSessionBanner(const char *application_dir_utf8)
{
	QString appdir = QString::fromUtf8(application_dir_utf8 ? application_dir_utf8 : "");
	const QString logpath = LC7DiagnosticLogResolvedPath();
	const QString ver = QString::fromLatin1(VERSION_STRING);

	const QString bannerPath =
		logpath.isEmpty() ? QStringLiteral("(no file, using stderr only)") : logpath;

	LC7DiagnosticLog(QStringLiteral("=== LC7 session start ===").toUtf8().constData());
	LC7DiagnosticLog(QStringLiteral("version %1").arg(ver).toUtf8().constData());
	LC7DiagnosticLog(QStringLiteral("applicationDir=%1").arg(appdir).toUtf8().constData());
	LC7DiagnosticLog(QStringLiteral("diagnosticLog=%1").arg(bannerPath).toUtf8().constData());

	const QByteArray toStderr = QStringLiteral("LC7 diagnostic log: %1").arg(bannerPath).toUtf8();
	lc7_diag_stderr(toStderr.constData());
}

}
