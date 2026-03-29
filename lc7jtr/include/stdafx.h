#pragma once

#if defined(_DEBUG) || defined(__APPLE__) || defined(__linux__)
// On macOS and Linux we load the jtrdll shim in-process (no external john binary)
#define CLC7JTREXEDLL CLC7JTRDLL
#else
#define CLC7JTREXEDLL CLC7JTREXE
#endif

#include <QtWidgets>
#include <QApplication>
#include <QResource>
#include <qdatetime.h>

#include"../../lc7core/include/platform_specific.h"

#include"../../lc7/include/appversion.h"
#include"lc7api.h"

#include"linkage.h"

#include"uuids.h"

#ifndef _MAX_PATH
#include <limits.h>
#define _MAX_PATH PATH_MAX
#endif

#define JTRDLL_IMPORTS
#include"jtrdll.h"

#include"CLC7JTR.h"
#include"CSystemJTR.h"
#include"CLC7JTRConsole.h"
#include"CTechniqueJTR.h"
#include"CTechniqueJTRBruteGUI.h"
#include"CTechniqueJTRDictionaryGUI.h"
#include"CLC7JTRPlugin.h"
#include"CLC7JTRDLL.h"
#include"CLC7JTREXE.h"
#include"CLC7ExecuteJTR.h"
#include"CLC7CharsetEditor.h"
#include"CLC7CharsetEditorDlg.h"
#include"CLC7JTRGPUManager.h"
#include"CLC7JTRPasswordEngine.h"
#include"jtrworker.h"
#include"jtrnodeworker.h"
#include"turbo_linecount.h"
