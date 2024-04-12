#pragma once
#include "Prerequisites.h"

namespace cppfd {
class Utils {
 public:
  static void ExeceptionLog(const String strExecptInfo);
  static bool IsCastable(const std::type_info& from_cls, const std::type_info& to_cls, void* objptr = 0);
  static bool StringStartWith(const String& inputString, const String& strPattern, bool bIgnoreCase = true);
  static bool StringEndWith(const String& inputString, const String& strPattern, bool bIgnoreCase = true);
  static void String2LowerCase(String& strString);
};
}