#include"stdafx.h"

#include"lc7_userinfo_wordlist.h"

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
#include<io.h>
#include<fcntl.h>
#include<crtdbg.h>
#endif



CJTRNodeWorker::CJTRNodeWorker(JTRWORKERCTX *ctx, JTRPASS *pass, int passnode, bool restore, QString potfilename, QString sessionfilename, QString hashesfilename) 
{TR;
	m_ctx = ctx;
	m_pass=pass;
	m_passnode=passnode;
	m_restore=restore;
	m_potfilename=potfilename;
	m_sessionfilename=sessionfilename;
	m_hashesfilename=hashesfilename;
	m_is_error=false;
	m_fallbackretry = false;
	
	m_exejtr = new CLC7ExecuteJTR(m_pass->nodes[m_passnode].jtrversion);
}
 
CJTRNodeWorker::~CJTRNodeWorker() 
{TR;
	delete m_exejtr;
	m_exejtr = NULL;
}

int CJTRNodeWorker::passnode()
{
	return m_passnode;
}

bool CJTRNodeWorker::fallbackretry()
{
	return m_fallbackretry;
}


bool CJTRNodeWorker::VerifyRestoreSession()
{TR;
	// Load up args from session file
	QStringList sessionargs;
	QFile infile(m_sessionfilename+".rec");
	if(!infile.open(QIODevice::ReadOnly))
	{
		return false;
	}
	QTextStream in(&infile);
	while(!in.atEnd())
	{
		sessionargs.append(in.readLine());
	}
	
	// Trim down to just args
	if(sessionargs[0]=="REC4")
	{
		sessionargs.removeFirst();
		int count=sessionargs[0].toInt();
		sessionargs=sessionargs.mid(1,count-1);
	}
	else
	{
		Q_ASSERT(0);
		return false;
	}

	// JtR session args must match at least our command line
	// If JtR adds extra arguments, it's okay too.
	foreach(QString arg, m_args)
	{
		if(!sessionargs.contains(arg))
		{
			return false;
		}
	}

	return true;
}

/*
static QString ToShortName(QString path)
{
	std::wstring wstr = path.toStdWString();

	DWORD length = GetShortPathNameW(wstr.c_str(), NULL, 0);
	if (length == 0)
		return path;

	wchar_t *buffer = new wchar_t[length];
	length = GetShortPathNameW(wstr.c_str(), buffer, length);
	if (length == 0)
	{
		delete[] buffer;
		return path;
	}
	
	QString ret = QString::fromWCharArray(buffer);
	delete[] buffer;
	return ret;
}
*/

bool CJTRNodeWorker::GenerateCommandLine(bool preflight)
{TR;
	m_args.clear();
	m_args
		<< "--nolog"
#ifdef _DEBUG
		<< "--verbosity=5"
#endif
//		<< "--skip-self-tests"
		;
	
	if (!preflight)
	{
		m_args << QString("--session=%1").arg(QDir::toNativeSeparators(m_sessionfilename));
		m_args << QString("--pot=%1").arg(QDir::toNativeSeparators(m_potfilename));
		m_args << QString("--format=%1").arg(m_pass->nodes[m_passnode].node_algorithm);

		if (m_pass->nodes[m_passnode].gpu_enable)
		{
			m_args << QString("--devices=%1").arg(m_pass->nodes[m_passnode].gpu_jtrindex);
		}
	}
	else
	{
		m_args << QString("--format=%1").arg(m_pass->nodes[m_passnode].preflight_node_algorithm);
	}

	if(m_pass->nodes.size()>1)
	{
		m_args << QString("--node=%1/%2").arg(m_passnode+1).arg(m_pass->nodes.size());
	}

	if (m_pass->jtrmode == "single")
	{
		QString wl = QDir(m_ctx->m_temporary_dir).filePath(QString("userinfo_candidates_%1.txt").arg(m_passnode));
		QString genErr;
		if (!lc7WriteUserInfoWordlist(QDir::toNativeSeparators(m_hashesfilename),
				QDir::toNativeSeparators(wl), &genErr))
		{
			set_error(genErr);
			return false;
		}
		m_args << QString("--wordlist=%1").arg(QDir::toNativeSeparators(wl))
			<< QString("--encoding=%1").arg(QStringLiteral("UTF-8"))
			<< QString("--rules=%1").arg(QStringLiteral("best64"));
	}
	else if (m_pass->jtrmode == "wordlist")
	{
		m_args << QString("--wordlist=%1").arg(QDir::toNativeSeparators(m_pass->wordlist_file))
			<< QString("--encoding=%1").arg(m_pass->encoding)
			<< QString("--rules=%1").arg(m_pass->rule);
		if (m_pass->leet)
		{
			m_args << QString("--external=HybridLeet");
		}
	}
	else if(m_pass->jtrmode=="incremental")
	{
		m_args << QString("--incremental=file:%1").arg(QDir::toNativeSeparators(m_pass->character_set));
		m_args << QString("--min-length=%1").arg(m_pass->num_chars_min);
		m_args << QString("--max-length=%1").arg(m_pass->num_chars_max);
		m_args << QString("--internal-codepage=%1").arg(m_pass->encoding);
	}
	else if (m_pass->jtrmode == "mask")
	{
		if (m_pass->num_chars_min == m_pass->num_chars_max)
		{
			m_args << QString("--mask=%1").arg(m_pass->mask.repeated(m_pass->num_chars_max));
		}
		else
		{
			m_args << QString("--mask=%1").arg(m_pass->mask);
			m_args << QString("--min-length=%1").arg(m_pass->num_chars_min);
			m_args << QString("--max-length=%1").arg(m_pass->num_chars_max);
		}
		m_args << QString("--internal-codepage=%1").arg(m_pass->encoding);
	}
	else
	{
		Q_ASSERT(false);
		return false;
	}
	
	/*
	
	Don't bother doing time restriction here
	
	if (!m_ctx->m_duration_unlimited)
	{
		// Get total max run time in seconds
		int maxruntime = (m_ctx->m_duration_hours * 3600) + (m_ctx->m_duration_minutes * 60);

		// Take out the seconds spent in previous passes
		maxruntime -= m_ctx->m_elapsed_seconds_at_start_of_pass;

		// Divide time out by the remaining passes
		// This ensures each pass gets time to run, favoring the later passes.
		// xxx: yes, this is totally inaccurate
		//maxruntime /= (m_ctx->m_jtrpasses.size() - m_ctx->m_current_pass);

		m_args << QString("--max-run-time=%1").arg(maxruntime);
	}
	*/

	QString path = QDir::toNativeSeparators(m_hashesfilename);


	m_args << path;

	return true;
}


bool CJTRNodeWorker::ExecuteJTRCommandLine()
{
	TR;
	m_engine_stderr_tail.clear();

	if (!m_exejtr->IsValid())
	{
		set_error("No version of the cracking engine is compatible with your system. Using a more modern CPU is required.");
		return false;
	}

#ifdef _DEBUG
	g_pLinkage->GetGUILinkage()->AppendToActivityLog(QString("ExecuteJTR: %1").arg(m_args.join("\n")));
#endif

	m_exejtr->SetCommandLine(m_args, m_pass->nodes[m_passnode].extra_opencl_kernel_args);

	int retval;
	if ((retval = m_exejtr->ExecutePipe(this)) != 0)
	{
		QString instructionset;
		if (m_exejtr->HadIllegalInstruction())
		{
			if (m_pass->nodes[m_passnode].gpu_enable)
			{
				set_error(QString("The GPU performed an illegal operation. Your GPU may not be supported by L0phtCrack at this time. Contact support@l0phtcrack.com."));
				return false;
			}
			else
			{
				// Try a different instruction set
				g_pLinkage->GetGUILinkage()->AppendToActivityLog(QString("Rejecting instruction set '%1'. Falling back...").arg(m_pass->nodes[m_passnode].jtrversion));
				CLC7JTR::DisableInstructionSet(m_pass->nodes[m_passnode].jtrversion);
				m_fallbackretry = true;
			
				return false;
			}
		}
		if (!isInterruptionRequested())
		{
			QString tail = m_engine_stderr_tail.trimmed();
			if (!tail.isEmpty())
				set_error(QString("Error code: %1\n%2").arg(retval).arg(tail));
			else
				set_error(QString("Error code: %1").arg(retval));
			return false;
		}
	}

	return true;
}

bool CJTRNodeWorker::preflight(CLC7ExecuteJTR::PREFLIGHT & preflight)
{
	if (!GenerateCommandLine(true))
	{
		if (!m_is_error)
			set_error("Couldn't generate command line.");
		return false;
	}

	if (!m_exejtr->IsValid())
	{
		set_error("No version of the cracking engine is compatible with your system. Using a more modern CPU is required.");
		return false;
	}

#ifdef _DEBUG
	g_pLinkage->GetGUILinkage()->AppendToActivityLog(QString("PreflightJTR: %1").arg(m_args.join("\n")));
#endif

	m_exejtr->SetCommandLine(m_args, m_pass->nodes[m_passnode].extra_opencl_kernel_args);

	m_exejtr->Preflight(preflight);
	if (!preflight.valid)
	{
		set_error("Preflight was invalid.");
		return false;
	}

	QString instructionset;
	if (m_exejtr->HadIllegalInstruction())
	{
		set_error("Preflight was invalid.");
		return false;
	}

	return true;
}

void CJTRNodeWorker::run() 
{TR;
	if(!GenerateCommandLine(false))
	{
		if (!m_is_error)
			set_error("Couldn't generate command line.");
		return;
	}
	
	// Verify restore session args
	if(m_restore)
	{
		if(!VerifyRestoreSession())
		{
			set_error("Configuration has changed, session can not be restarted.");
			return;
		}

		// Restore is valid
		m_args.clear();
		m_args 
			<< "--nolog" 
#ifdef _DEBUG
			<< "--verbosity=5"
#endif
			<< "--skip-self-tests"
			<< QString("--restore=%1").arg(QDir::toNativeSeparators(m_sessionfilename));
	}
			
	if(!ExecuteJTRCommandLine())
	{
		return;
	}
}

void CJTRNodeWorker::stop(bool timeout)
{TR;
	requestInterruption();

	// Tell JTR to stop
	if (m_exejtr)
	{
		m_exejtr->Abort(timeout);
	}
	
	wait();
}

JTRDLL_STATUS CJTRNodeWorker::get_status()
{
	if (!m_exejtr)
	{
		JTRDLL_STATUS jtrdllstatus;
		memset(&jtrdllstatus, 0, sizeof(jtrdllstatus));
		return jtrdllstatus;
	}

	JTRDLL_STATUS status = m_exejtr->GetStatus();

	m_etas.push_back(status.eta);
	if(m_etas.length()==11)
	{
		m_etas.pop_front();
	}
	unsigned int avg_eta=0;
	foreach(unsigned int eta, m_etas)
	{
		avg_eta+=eta;
	}
	avg_eta/=m_etas.length();

	status.eta = avg_eta;

	return status;
}

bool CJTRNodeWorker::lasterror(QString &err)
{TR;
	if(!m_is_error)
	{
		return false;
	}
	err=m_error;
	return true;
}

void CJTRNodeWorker::set_error(QString err)
{
	m_error=err;
	m_is_error=true;
}


static QString filterPrintable(QString str)
{
	QString out;
	foreach(QChar c, str)
	{
		if (c.isPrint() || c.isSpace())
		{
			out += c;
		}
	}
	return out;
}

static QString engineLogVerbosity()
{
	// Force compact output for now, until explicit UI toggle is added.
	return "results_only";
}

static bool isResultLine(const QString &line)
{
	const QString l = line.toLower();
	return l.contains("recovered") || l.contains("cracked") || l.contains("found");
}

static bool isStatusLine(const QString &line)
{
	const QString l = line.toLower();
	return l.contains("eta") || l.contains("speed") || l.contains("progress") || l.contains("pass ");
}

static bool isDebugNoiseLine(const QString &line)
{
	const QString l = line.toLower();
	return l.contains("opencl") || l.contains("platform") || l.contains("device #") ||
		l.contains("kernel") || l.contains("loaded ") || l.contains("rules") ||
		l.contains("using ") || l.contains("warning:");
}

static bool shouldShowEngineLine(const QString &line)
{
	const QString verbosity = engineLogVerbosity();
	if (verbosity == "all")
	{
		return true;
	}
	if (verbosity == "status")
	{
		return isResultLine(line) || isStatusLine(line);
	}

	// results_only (default)
	if (isDebugNoiseLine(line))
	{
		return false;
	}
	return isResultLine(line);
}

static QStringList extractRecoveredPairsFromChunk(const QString &chunk)
{
	QStringList pairs;
	QSet<QString> seen;

	// Typical hashcat recovered token: <hash>:<password>
	QRegularExpression re_pair("([0-9A-Fa-f]{32,}:[^\\s\\[]+)");
	QRegularExpressionMatchIterator it = re_pair.globalMatch(chunk);
	while (it.hasNext())
	{
		QRegularExpressionMatch m = it.next();
		QString token = m.captured(1).trimmed();
		if (!token.isEmpty() && !seen.contains(token))
		{
			seen.insert(token);
			pairs.append(token);
		}
	}

	return pairs;
}

void CJTRNodeWorker::ProcessStdOut(QByteArray line)
{
	QString out = filterPrintable(QString::fromUtf8(line));
	if (out.trimmed().size() == 0)
	{
		return;
	}
	QStringList recovered = extractRecoveredPairsFromChunk(out);
	foreach(QString rec, recovered)
	{
		QString msg = rec;
		if (m_pass->nodes.size() > 1)
		{
			msg = QString("Node %1: ").arg(m_passnode + 1) + msg;
		}
		g_pLinkage->GetGUILinkage()->AppendToActivityLog(msg + "\n");
	}
	if (recovered.isEmpty())
	{
		TRDBG(QString("Hashcat stdout suppressed: %1").arg(out.trimmed()).toUtf8().constData());
	}
}

void CJTRNodeWorker::ProcessStdErr(QByteArray line)
{
	QString err = filterPrintable(QString::fromUtf8(line));
	if (err.trimmed().size() == 0)
	{
		return;
	}
	QStringList recovered = extractRecoveredPairsFromChunk(err);
	foreach(QString rec, recovered)
	{
		QString msg = rec;
		if (m_pass->nodes.size() > 1)
		{
			msg = QString("Node %1: ").arg(m_passnode + 1) + msg;
		}
		g_pLinkage->GetGUILinkage()->AppendToActivityLog(msg + "\n");
	}
	if (recovered.isEmpty())
	{
		QString t = err.trimmed();
		if (!t.isEmpty())
		{
			m_engine_stderr_tail += t;
			m_engine_stderr_tail += QLatin1Char('\n');
			const int cap = 6000;
			if (m_engine_stderr_tail.size() > cap)
				m_engine_stderr_tail = m_engine_stderr_tail.right(cap);
		}
		TRDBG(QString("Hashcat stderr suppressed: %1").arg(err.trimmed()).toUtf8().constData());
	}
}
