#pragma once

#ifdef WIN32

#include <io.h>
#include <process.h>

typedef unsigned long ssize_t;

#define alloca _alloca
#define open _open
#define write _write
#define read _read
#define close _close
#define getpid _getpid

#pragma warning (disable: 4996 6255 6263)

#pragma warning (disable: 6031)

#endif
