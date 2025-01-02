#pragma once
#include "Prerequisites.h"
#include "Utils.h"

namespace cppfd {
enum ELogLevel {
  ELogLevel_Fatal = 0,
  ELogLevel_Error,
  ELogLevel_Warning,
  ELogLevel_Info,
  ELogLevel_Debug,

  ELogLevel_Num,
};

enum ELogCut {
	ELogCut_Day,
	ELogCut_Hour,
	ElogCut_Minute,
};

struct LogConfig {
  String Path;
  String FileName;
  int LogLevel;
  uint32_t MaxSize;  //MB
  int FileCut;
  uint32_t KeepDays;
  String ProcessName;

  LogConfig() : Path("./log"), FileName("cppfd_log"), LogLevel(ELogLevel_Info), MaxSize(100), FileCut(ELogCut_Day), KeepDays(3) {}
};

class Log {
 public:
  static bool Init(const LogConfig& cfg);
  static void Destroy();
  static void WriteLog(ELogLevel eLogLevel, const char* szFormat, ...);
  static const char* GetFileName(const char* szFileName);
  static const char* GetFuncName(const char* szFuncName);
  static void FlushAndAbort();
};

#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
#  define LOG_WARNING(fmt, ...) cppfd::Log::WriteLog(cppfd::ELogLevel_Warning, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), __VA_ARGS__)

#  define LOG_FATAL(fmt, ...)                                                                                                                                               \
    do {                                                                                                                                                                    \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Fatal, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), __VA_ARGS__); \
      cppfd::Log::FlushAndAbort();                                                                                                                                          \
    } while (0)

#  define LOG_ERROR(fmt, ...) cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), __VA_ARGS__)

#  define LOG_TRACE(fmt, ...) cppfd::Log::WriteLog(cppfd::ELogLevel_Info, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), __VA_ARGS__)

#  define LOG_DEBUG(fmt, ...) cppfd::Log::WriteLog(cppfd::ELogLevel_Debug, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), __VA_ARGS__)

#  define Assert(expr, fmt, ...)                                                                                                                                                       \
    if (!(expr)) {                                                                                                                                                                     \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s][%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), #expr, __VA_ARGS__); \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Fatal, "%s", cppfd::Utils::GetCallStack().c_str());                                                                                        \
      cppfd::Log::FlushAndAbort();                                                                                                                                                     \
    }

#  define AssertNoAbort(expr, ret, fmt, ...)                                                                                                                                           \
    if (!(expr)) {                                                                                                                                                                     \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s][%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), #expr, __VA_ARGS__); \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "%s", cppfd::Utils::GetCallStack().c_str());                                                                                        \
      return ret;                                                                                                                                                                      \
    }

#  define AssertNoAbortV(expr, fmt, ...)                                                                                                                                               \
    if (!(expr)) {                                                                                                                                                                     \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s][%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), #expr, __VA_ARGS__); \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "%s", cppfd::Utils::GetCallStack().c_str());                                                                                        \
      return;                                                                                                                                                                          \
    }

#  define AssertContinue(expr, fmt, ...)                                                                                                                                               \
    if (!(expr)) {                                                                                                                                                                     \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s][%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), #expr, __VA_ARGS__); \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "%s", cppfd::Utils::GetCallStack().c_str());                                                                                        \
      continue;                                                                                                                                                                        \
    }

#else
#  define LOG_WARNING(fmt, _arg...) cppfd::Log::WriteLog(cppfd::ELogLevel_Warning, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), ##_arg)

#  define LOG_FATAL(fmt, _arg...)                                                                                                                                      \
    do {                                                                                                                                                               \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Fatal, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), ##_arg); \
      cppfd::Log::FlushAndAbort();                                                                                                                                     \
    } while (0)

#  define LOG_ERROR(fmt, _arg...) cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), ##_arg)

#  define LOG_TRACE(fmt, _arg...) cppfd::Log::WriteLog(cppfd::ELogLevel_Info, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), ##_arg)

#  define LOG_DEBUG(fmt, _arg...) cppfd::Log::WriteLog(cppfd::ELogLevel_Debug, "[%s:%d][fun:%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), ##_arg)

#  define Assert(expr, fmt, _arg...)                                                                                                                                              \
    if (!(expr)) {                                                                                                                                                                \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s][%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), #expr, ##_arg); \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Fatal, "%s", cppfd::Utils::GetCallStack().c_str());                                                                                   \
      cppfd::Log::FlushAndAbort();                                                                                                                                                \
    }

#  define AssertNoAbort(expr, ret, fmt, _arg...)                                                                                                                                  \
    if (!(expr)) {                                                                                                                                                                \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s][%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), #expr, ##_arg); \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "%s", cppfd::Utils::GetCallStack().c_str());                                                                                   \
      return ret;                                                                                                                                                                 \
    }

#  define AssertNoAbortV(expr, fmt, _arg...)                                                                                                                                      \
    if (!(expr)) {                                                                                                                                                                \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s][%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), #expr, ##_arg); \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "%s", cppfd::Utils::GetCallStack().c_str());                                                                                   \
      return;                                                                                                                                                                     \
    }

#  define AssertContinue(expr, fmt, _arg...)                                                                                                                                      \
    if (!(expr)) {                                                                                                                                                                \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "[%s:%d][fun:%s][%s]" fmt, cppfd::Log::GetFileName(__FILE__), __LINE__, cppfd::Log::GetFuncName(__FUNCTION__), #expr, ##_arg); \
      cppfd::Log::WriteLog(cppfd::ELogLevel_Error, "%s", cppfd::Utils::GetCallStack().c_str());                                                                                   \
      continue;                                                                                                                                                                   \
    }

#endif
}

