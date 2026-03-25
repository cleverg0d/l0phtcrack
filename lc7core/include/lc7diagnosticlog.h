#ifndef __INC_LC7DIAGNOSTICLOG_H
#define __INC_LC7DIAGNOSTICLOG_H

#include "platform_specific.h"

#include <QString>

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT void LC7DiagnosticLog(const char *utf8_line);
DLLEXPORT void LC7DiagnosticLogSessionBanner(const char *application_dir_utf8);

#ifdef __cplusplus
}
#endif

QString LC7DiagnosticLogResolvedPath(void);

#endif
