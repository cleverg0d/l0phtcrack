#include "stdafx.h"

#include "lc7_userinfo_wordlist.h"

#include <QFile>
#include <QRegularExpression>

namespace {

static void writeUtf8Line(QFile &fout, const QString &s)
{
	const QByteArray line = s.toUtf8();
	fout.write(line.constData(), line.size());
	fout.write("\n", 1);
}

static void emitWord(QFile &fout, const QString &word)
{
	if (word.size() < 2)
		return;

	static const char *const sfx[] = {
		"", "1", "2", "3", "12", "21", "123", "321", "1234", "12345", "123456",
		"01", "02", "99", "00", "007", "111", "222", "333", "!", "@", "#",
		"1!", "2@", "123!", "!@#", "2023", "2024", "2025", "_1", "_123", nullptr
	};

	QString low = word.toLower();
	QString cap = low;
	if (!cap.isEmpty())
		cap[0] = cap[0].toUpper();

	const QString up = word.toUpper();

	for (int s = 0; sfx[s]; s++)
	{
		const QString suf = QString::fromUtf8(sfx[s]);
		writeUtf8Line(fout, cap + suf);
		if (low != cap)
			writeUtf8Line(fout, low + suf);
		if (up != cap && up != low)
			writeUtf8Line(fout, up + suf);
	}
}

static void splitAndEmit(QFile &fout, const QString &str, const QString &delimClass, int minLen)
{
	if (str.isEmpty())
		return;
	static const QRegularExpression re(QStringLiteral("[%1]").arg(QRegularExpression::escape(delimClass)));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	const QStringList parts = str.split(re, Qt::SkipEmptyParts);
#else
	const QStringList parts = str.split(re, QString::SkipEmptyParts);
#endif
	for (const QString &tok : parts)
	{
		if (tok.length() >= minLen)
			emitWord(fout, tok);
	}
}

} // namespace

bool lc7WriteUserInfoWordlist(const QString &jtrHashFilePath, const QString &outWordlistPath, QString *errorOut)
{
	QFile fin(jtrHashFilePath);
	if (!fin.open(QIODevice::ReadOnly))
	{
		if (errorOut)
			*errorOut = QStringLiteral("Cannot open hash file for User Info wordlist.");
		return false;
	}

	QFile fout(outWordlistPath);
	if (!fout.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		if (errorOut)
			*errorOut = QStringLiteral("Cannot write User Info wordlist.");
		return false;
	}

	int lines = 0;
	while (!fin.atEnd())
	{
		QByteArray lineBytes = fin.readLine();
		if (lineBytes.isEmpty())
			continue;
		if (lineBytes.endsWith('\n'))
			lineBytes.chop(1);
		if (lineBytes.endsWith('\r'))
			lineBytes.chop(1);
		if (lineBytes.isEmpty())
			continue;

		const QString line = QString::fromUtf8(lineBytes);

		const int p0 = line.indexOf(QLatin1Char(':'));
		if (p0 <= 0)
			continue;

		const QString username = line.left(p0);
		if (username.isEmpty())
			continue;

		const int pGe = line.indexOf(QStringLiteral(":::"), p0 + 1);
		if (pGe < 0)
			continue;
		const int contentStart = pGe + 3;
		const int pEnd = line.indexOf(QStringLiteral("::"), contentStart);
		if (pEnd < 0)
			continue;

		const QString fullname = line.mid(contentStart, pEnd - contentStart);

		emitWord(fout, username);
		splitAndEmit(fout, username, QStringLiteral("._-"), 3);

		if (!fullname.isEmpty())
		{
			emitWord(fout, fullname);
			splitAndEmit(fout, fullname, QStringLiteral(" .-_"), 3);
		}
		lines++;
	}

	fout.close();
	fin.close();

	if (lines <= 0)
	{
		if (errorOut)
			*errorOut = QStringLiteral("No hash lines parsed for User Info wordlist.");
		return false;
	}

	return true;
}
