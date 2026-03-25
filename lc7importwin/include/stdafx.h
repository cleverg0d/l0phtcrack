#pragma once

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <QtWidgets>
#include <QApplication>
#include <QResource>
#include <qdatetime.h>

#include"../../lc7core/include/platform_specific.h"

#include"../../lc7/include/appversion.h"
#include"lc7api.h"

#include"linkage.h"

#include"windows_abstraction.h"
#include"uuids.h"

#ifndef __APPLE__
#include"PubKeyFile.h"
#endif
#include"CLC7ImportWinPlugin.h"
#include"CImportWindows.h"
#include"PWDumpImporter.h"
#include"SAMImporter.h"
#include"CImportPWDump.h"
#include"CImportPWDumpGUI.h"
#include"CImportSAM.h"
#include"CImportSAMGUI.h"

// NTDS.DIT import — available on all platforms (libesedb on macOS, ESENT on Windows)
#include"NTDSImporter.h"
#include"CImportNTDS.h"
#include"CImportNTDSGUI.h"

#ifndef __APPLE__
// Windows-only: DRSR (requires RPC/SSPI), local/remote Windows system imports
#include"DRSRImporter.h"
#include"CImportWindowsLocal.h"
#include"CImportWindowsLocalGUI.h"
#include"CImportWindowsRemote.h"
#include"CImportWindowsRemoteGUI.h"
#include"GenerateRemoteAgentDlg.h"
#endif

#include"CWindowsImportSettings.h"
