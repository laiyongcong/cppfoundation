#pragma once
#include "Prerequisites.h"
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
#include <WinSock2.h>
#else
#include <sys/time.h>
#endif

namespace cppfd {
class Utils {
 public:
  static void ExeceptionLog(const String strExecptInfo);
  static bool StringStartWith(const String& inputString, const String& strPattern, bool bIgnoreCase = true);
  static bool StringEndWith(const String& inputString, const String& strPattern, bool bIgnoreCase = true);
  static void String2LowerCase(String& strString);
  static void StringExcape(const String& strInput, String& strRes);
  static void StringUnExcape(const String& strInput, String& strRes);
  static String Double2String(double dNumber, uint32_t uDigit);
  static int GetTimeOfDay(struct timeval* tp, void* tzp);
};
}