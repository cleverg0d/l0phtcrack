#include "stdafx.h"
#include "LC7Main.h"
#include <math.h>
#include <cstdio>

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
#include"crtdbg.h"
#include"CrashRpt.h"
#endif

bool g_CrashRptInstalled=false;

void InstallCrashRpt(void)
{
#if PLATFORM == PLATFORM_WIN64
	_set_FMA3_enable(0);
#endif

#if ((PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)) && !defined(_DEBUG)
	 // Install crash reporting

    CR_INSTALL_INFO info;
    memset(&info, 0, sizeof(CR_INSTALL_INFO));
    info.cb = sizeof(CR_INSTALL_INFO);				// Size of the structure
    info.pszAppName = "L0phtCrack 7";				// App name
	info.pszAppVersion = VERSION_STRING;				// App version
	info.dwFlags = CR_INST_AUTO_THREAD_HANDLERS | CR_INST_ALL_POSSIBLE_HANDLERS;
	info.pszEmailSubject = "L0phtCrack 7 v" VERSION_STRING " Error Report"; // Email subject
	info.uMiniDumpType = (MINIDUMP_TYPE) (MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithProcessThreadData | MiniDumpWithThreadInfo);
    info.pszEmailTo = "crashdump@securixy.kz";   // Email recipient address
	info.pszSmtpProxy = "mailproxy.securixy.kz:465"; // SMTP Server
	info.uPriorities[CR_HTTP]=CR_NEGATIVE_PRIORITY;
	info.uPriorities[CR_SMTP]=2;
	info.uPriorities[CR_SMAPI]=1;

    // Install crash handlers
    int nInstResult = crInstall(&info);

	// Check result
    Q_ASSERT(nInstResult==0);

	if(nInstResult==0)
	{
		g_CrashRptInstalled=true;
	}
#endif
}

void UninstallCrashRpt(void)
{
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	if(!g_CrashRptInstalled)
	{
		return;
	}
	int nUninstRes = crUninstall(); // Uninstall exception handlers
    Q_ASSERT(nUninstRes==0);
#else

#endif
}

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
#include <stdio.h>
void RedirectIOToConsole()
{
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		freopen("CONOUT$", "wb", stdout);
		freopen("CONOUT$", "wb", stderr);
	}
}

#else
void RedirectIOToConsole()
{
    // No-op on non-Windows platforms; output goes to terminal by default
}
#endif



int main(int argc, char *argv[])
{
	qRegisterMetaTypeStreamOperators<LC7SecureString>("LC7SecureString");
	
	//int tmp=_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	//tmp |= _CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_CRT_DF|_CRTDBG_DELAY_FREE_MEM_DF|_CRTDBG_CHECK_EVERY_1024_DF;//|_CRTDBG_CHECK_ALWAYS_DF; //|_CRTDBG_LEAK_CHECK_DF;// | _CRTDBG_CHECK_EVERY_16_DF;
	//_CrtSetDbgFlag(tmp);

	//	_CrtSetBreakAlloc(18027);
	RedirectIOToConsole();

#ifdef __linux__
    // On Linux, prevent loading SYSTEM Qt theme/style/inputcontext plugins.
    // Even with qt.conf isolating platform plugins, Qt still searches
    // /usr/lib/.../qt5/plugins/{platformthemes,styles,platforminputcontexts}
    // and loads whatever it finds — compiled against SYSTEM Qt Core,
    // NOT our bundled Qt Core → ABI mismatch → SIGABRT on any machine where
    // system Qt != our Qt 5.15.13 (built on Ubuntu 24.04).
    // Belt+suspenders with lc7.sh, but needed here for direct ./lc7 invocation too.
    if (!getenv("QT_QPA_PLATFORMTHEME"))
        setenv("QT_QPA_PLATFORMTHEME", "", 1);    // no system theme plugin
    if (!getenv("QT_STYLE_OVERRIDE"))
        setenv("QT_STYLE_OVERRIDE", "Fusion", 1); // built-in style, no plugin needed
    if (!getenv("QT_IM_MODULE"))
        setenv("QT_IM_MODULE", "none", 1);        // no system input method plugin
#endif

	CLC7App app(argc, argv);

	// Pass in argc/argv because for some reason there's a race condition.
	app.ParseCommandLine();

	QCommandLineParser & parser = app.GetCommandLineParser();
	
	if (app.isRunning())
	{
		if (!parser.value("task").isEmpty())
		{
			return !app.sendMessage("TASK:" + parser.value("task"));
		}
		if (!parser.value("run").isEmpty())
		{
			QString runmsg = "RUN:" + parser.value("task");
			if (!parser.value("save").isEmpty())
			{
				runmsg += "," + parser.value("save");
			}
			return !app.sendMessage(runmsg);
		}
		if (parser.positionalArguments().size()==1)
		{
			return !app.sendMessage("OPEN:" + parser.positionalArguments()[0]);
		}

		/* Finder / Dock second launch: primary instance must be pinged or it looks like "nothing happens". */
		if (!app.sendMessage(QStringLiteral("ACTIVATE"), 5000))
		{
			std::fprintf(stderr,
				"LC7: no response from the running instance (stale lock?). Quit LC7 or run: pkill lc7\n");
			std::fflush(stderr);
		}
		return 0;
	}
	
	InstallCrashRpt();
	
	bool normal_startup = true;
	if (!app.DoAttachManager(normal_startup))
	{
		UninstallCrashRpt();
		return 4;
	}
	if (!normal_startup)
	{
		UninstallCrashRpt();
		return 0;
	}

	if(!app.Initialize())
	{
		if(!app.Terminate())
		{
			UninstallCrashRpt();
			return 3;
		}
		UninstallCrashRpt();
		return 1;
	}

	int retcode=0;
	retcode=app.Execute();

	if(!app.Terminate())
	{
		UninstallCrashRpt();
		return 2;
	}

	app.quit();

	UninstallCrashRpt();
	
	return retcode;
}
