#include "Utils.h"
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
#  include <windows.h>
#  pragma warning(disable : 4091)
#  include <DbgHelp.h>
#  include <direct.h>
#  include <io.h>
#  pragma comment(lib, "Dbghelp.lib")
#  pragma warning(default : 4091)
#  include <MMSystem.h>
#  include <WinSock2.h>
#  include <process.h>
#  pragma comment(lib, "winmm")
#else
#  include <execinfo.h>
#  include <fcntl.h>
#  include <unistd.h>
#  include <sys/select.h>
#  include<sys/time.h>
#  include <netdb.h>
#  include <arpa/inet.h>
#  include <errno.h>
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

void Utils::StringExcape(const String& strInput, String& strRes) {
  static std::map<char, String> escapeChars = {
      {'\\', "\\\\"}, 
      {'\'', "\\'"}, 
      {'\"', "\\\""}, 
      {'\n', "\\n"}, 
      {'\r', "\\r"}, 
      {'\t', "\\t"}, 
      {'\b', "\\b"},
      {'\f', "\\f"},  
      {'\v', "\\v"}, 
      {'\a', "\\a"},  
      //{'{', "\\{"},  
      //{'}', "\\}"},  
      //{'[', "\\["},  
      //{']', "\\]"}
  };

  for (char ch : strInput) {
    if (escapeChars.count(ch)) {
      strRes.append(escapeChars[ch]);
    } else {
      strRes.append(1, ch);
    }
  }
 }

void Utils::StringUnExcape(const String& strInput, String& strRes) {
  static std::map<String, char> unescapeMap = {
      {"\\\\", '\\'}, 
      {"\\'", '\''}, 
      {"\\\"", '\"'}, 
      {"\\n", '\n'}, 
      {"\\r", '\r'}, 
      {"\\t", '\t'}, 
      {"\\b", '\b'},
      {"\\f", '\f'},  
      {"\\v", '\v'}, 
      {"\\a", '\a'},  
      //{"\\{", '{'},  
      //{"\\}", '}'},  
      //{"\\[", '['},  
      //{"\\]", ']'}
  };

  for (size_t i = 0; i < strInput.size(); ++i) {
    if (strInput[i] == '\\' && i + 1 < strInput.size()) {
      String escapeSeq = strInput.substr(i, 2);
      if (unescapeMap.count(escapeSeq)) {
        strRes += unescapeMap[escapeSeq];
        i++;
      } else {
        strRes += strInput[i];
      }
    } else {
      strRes += strInput[i];
    }
  }
 }

String Utils::Double2String(double dNumber, uint32_t uDigit) {
  if (uDigit > 20) uDigit = 20;

  int64_t nIntPart;
  double doublePart;
  char szInt[64] = {0};
  char szDouble[64] = {0};
  nIntPart = (int64_t)dNumber;
  doublePart = dNumber - nIntPart;

  if (nIntPart == 0 && dNumber < 0.0) {
    safe_printf(szInt, sizeof(szInt), "-0");
  } else {
    safe_printf(szInt, sizeof(szInt), "%" PRId64, nIntPart);
  }
  doublePart = fabs(pow(10, uDigit) * doublePart);

  uint64_t uldoublePart = (uint64_t)doublePart;
  while (uldoublePart % 10 == 0 && uldoublePart > 0) {
    uldoublePart /= 10;
    uDigit--;
  }
  while (uDigit > 0) {
    szDouble[uDigit - 1] = '0' + uldoublePart % 10;
    uDigit--;
    uldoublePart /= 10;
  }

  String strRes(szInt);
  strRes.append(".");
  strRes.append(szDouble);
  return strRes;
 }

int Utils::GetTimeOfDay(struct timeval* tp, void* tzp) {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  int64_t now = 0;
  if (tzp != 0) {
    errno = EINVAL;
    return -1;
  }
  GetSystemTimeAsFileTime((LPFILETIME)&now);
  tp->tv_sec = (long)(now / 10000000 - 11644473600LL);
  tp->tv_usec = (now / 10) % 1000000;
  return 0;
#else
  return gettimeofday(tp, (struct timezone*)tzp);
#endif
}

uint64_t Utils::GetTimeMiliSec() {
  timeval tv;
  if (GetTimeOfDay(&tv, nullptr) != 0) return 0;
  return ((uint64_t)tv.tv_usec) / 1000 + ((uint64_t)tv.tv_sec) * 1000;
}
}  // namespace cppfd