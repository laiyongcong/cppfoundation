#include "Utils.h"
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif  // !WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  pragma warning(disable : 4091)
#  include <DbgHelp.h>
#  include <direct.h>
#  include <io.h>
#  pragma comment(lib, "Dbghelp.lib")
#  pragma warning(default : 4091)
#else
#  include <execinfo.h>
#endif

namespace cppfd {

 void Utils::ExeceptionLog(const String strExecptInfo) {
  FILE* pFile = fopen("./Exeception.log", "ab");
  if (pFile) {
    fprintf(pFile, "Exeception:%s\nCallStack:\n", strExecptInfo.c_str());
    uint32_t i;
    void* stack[100] = {NULL};
    uint32_t frames;
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
    SYMBOL_INFO* symbol;
    HANDLE process;

    process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    frames = CaptureStackBackTrace(0, 100, stack, NULL);
    symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (i = 1; i < frames; i++) {
      SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
      char tmpBuff[512];
      safe_printf(tmpBuff, sizeof(tmpBuff), "\t%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address);
      uint32_t uLen = (uint32_t)strlen(tmpBuff);
      fwrite(tmpBuff, 1, uLen, pFile);
    }

    free(symbol);
    fclose(pFile);
#else
    fclose(pFile);
    int fd = open("./Exeception.log", O_CREAT | O_WRONLY, 0666);
    if (fd < 0) return;
    frames = backtrace(stack, 100);
    backtrace_symbols_fd(stack, frames, fd);
    close(fd);
#endif
  }
}

bool Utils::StringStartWith(const String& inputString, const String& strPattern, bool bIgnoreCase /*= true*/) {
  size_t nThisLen = inputString.length();
  size_t nPatternLen = strPattern.length();

  if (nThisLen < nPatternLen || nPatternLen == 0) return false;

  String strStartOfThis = inputString.substr(0, nPatternLen);

  if (bIgnoreCase) {
    String2LowerCase(strStartOfThis);
    String strNewPattern = strPattern;
    String2LowerCase(strNewPattern);
    return strStartOfThis == strNewPattern;
  }

  return (strStartOfThis == strPattern);
 }

bool Utils::StringEndWith(const String& inputString, const String& strPattern, bool bIgnoreCase /*= true*/) {
  size_t nThisLen = inputString.length();
  size_t nPatternLen = strPattern.length();

  if (nThisLen < nPatternLen || nPatternLen == 0) return false;

  String strEndOfThis = inputString.substr(nThisLen - nPatternLen, nPatternLen);

  if (bIgnoreCase) {
    String2LowerCase(strEndOfThis);
    String strNewPattern = strPattern;
    String2LowerCase(strNewPattern);
    return strEndOfThis == strNewPattern;
  }

  return (strEndOfThis == strPattern);
 }

 void Utils::String2LowerCase(String& strString) { std::transform(strString.begin(), strString.end(), strString.begin(), ::tolower); }

 }  // namespace cppfd