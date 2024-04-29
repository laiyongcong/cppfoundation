#pragma once

#include "PlatformDefine.h"
#include <stdint.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdarg>
#include <cmath>

// STL containers
#include <vector>
#include <map>
#include <string>
#include <set>
#include <list>
#include <deque>
#include <queue>
#include <bitset>
#include <memory>
#include <inttypes.h>

// STL algorithms & functions
#include <algorithm>
#include <functional>
#include <limits>

// C++ Stream stuff
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <typeinfo>
#include <stdexcept>
#include <cstddef>
#include <atomic>
#include <mutex>

extern "C" {
#include <sys/stat.h>
#include <sys/types.h>
}

#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
#  define tsnprintf _snprintf
#  define tvsnprintf _vsnprintf
#  define tstricmp _stricmp
#  define tstrnicmp _strnicmp

#  define safe_printf(DST, LEN, fmt, ...)        \
    do {                                         \
      tsnprintf(DST, (LEN)-1, fmt, __VA_ARGS__); \
      (DST)[(LEN)-1] = 0;                        \
    } while (0)
#else
#  define tsnprintf snprintf
#  define tvsnprintf vsnprintf
#  define tstricmp strcasecmp
#  define tstrnicmp strncasecmp

#  define safe_printf(DST, LEN, fmt, _arg...) \
    do {                                      \
      tsnprintf(DST, (LEN), fmt, ##_arg);     \
      (DST)[(LEN)-1] = 0;                     \
    } while (0)
#endif

#define CPPFD_DO_JOIN(X, Y) X##Y
#define CPPFD_JOIN(X, Y) CPPFD_DO_JOIN(X, Y)

#define CPPFD_OFFSET(C, M) ((unsigned long long)(&((const C*)1024)->M) - 1024)

template <bool>
struct CompileTimeError;
template <>
struct CompileTimeError<true> {};
#ifndef COMPILE_TIME_ASSERT
#  define COMPILE_TIME_ASSERT(expr) (CompileTimeError<(expr) != 0>())
#endif

namespace cppfd{
typedef std::string String;
typedef std::stringstream StringStream;
typedef std::istringstream iStringStream;
typedef std::ostringstream oStringStream;

class NonCopyable
{
 protected:
  NonCopyable() {}
  ~NonCopyable() {}

 private:
  NonCopyable(const NonCopyable&);
  const NonCopyable& operator=(const NonCopyable&);
};
}