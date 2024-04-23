#pragma once
#include "Prerequisites.h"

namespace cppfd {
class Utils {
 public:
  static void ExeceptionLog(const String strExecptInfo);
  static bool StringStartWith(const String& inputString, const String& strPattern, bool bIgnoreCase = true);
  static bool StringEndWith(const String& inputString, const String& strPattern, bool bIgnoreCase = true);
  static void String2LowerCase(String& strString);
  static void StringExcape(const String& strInput, String& strRes);
  static void StringUnExcape(const String& strInput, String& strRes);
  static String Double3String(double dNumber, uint32_t uDigit);
};
}