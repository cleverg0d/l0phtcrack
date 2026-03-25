#pragma once
// Portable Windows type definitions for non-Windows platforms
// Used by lc7importwin when building on macOS/Linux without Wine

#ifndef _WIN32

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef int      BOOL;
typedef int32_t  LONG;
typedef char     CHAR;
typedef wchar_t  WCHAR;

typedef char*          LPSTR;
typedef const char*    LPCSTR;
typedef wchar_t*       LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t*       LPTSTR;
typedef BYTE*          LPBYTE;
typedef void*          LPVOID;
typedef DWORD*         LPDWORD;

typedef unsigned int UINT;
typedef void*   HANDLE;
typedef void*   SC_HANDLE;
typedef HANDLE* PHANDLE;

#ifndef _countof
#define _countof(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)(-1))
#define INVALID_SOCKET       ((int)(-1))

// HKEY stubs — ntreg uses its own hive structure, not Windows registry APIs
typedef void*  HKEY;
#define HKEY_LOCAL_MACHINE  ((HKEY)(uintptr_t)0x80000002UL)
#define HKEY_CURRENT_USER   ((HKEY)(uintptr_t)0x80000001UL)

// Registry value types
#define REG_NONE              0
#define REG_SZ                1
#define REG_EXPAND_SZ         2
#define REG_BINARY            3
#define REG_DWORD             4
#define REG_DWORD_LITTLE_ENDIAN 4
#define REG_DWORD_BIG_ENDIAN  5
#define REG_LINK              6
#define REG_MULTI_SZ          7
#define REG_QWORD             11

// snprintf compat
#define _snprintf snprintf

// Calling convention / attribute macros
#ifndef WINAPI
#define WINAPI
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef MAXULONG_PTR
#define MAXULONG_PTR ((uintptr_t)~0ULL)
#endif

#include <string.h>
#define ZeroMemory(ptr, size) memset((ptr), 0, (size))
#define CopyMemory(dst, src, size) memcpy((dst), (src), (size))

// Stub macros for unimplemented Windows-only paths
#define UNIMPLEMENTED do {} while(0)
#define UNSUPPORTED   do {} while(0)

#endif // !_WIN32
