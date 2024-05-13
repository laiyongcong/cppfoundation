#include "Utils.h"
#include "Log.h"
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
#  include <windows.h>
#include <Psapi.h>
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
#  include <sys/param.h>
#  include <sys/stat.h>
#  include <ctype.h>
#  include <dirent.h>
#  include <fnmatch.h>
#  include <string.h>

/* Our simplified data entry structure */
struct _finddata_t {
  char* name;
  int attrib;
  unsigned long size;
};

#  define _A_NORMAL 0x00 /* Normalfile-Noread/writerestrictions */
#  define _A_RDONLY 0x01 /* Read only file */
#  define _A_HIDDEN 0x02 /* Hidden file */
#  define _A_SYSTEM 0x04 /* System file */
#  define _A_ARCH 0x20   /* Archive file */
#  define _A_SUBDIR 0x10 /* Subdirectory */

struct _find_search_t {
  char* pattern;
  char* curfn;
  char* directory;
  int dirlen;
  DIR* dirfd;
};

int _findnext(intptr_t id, struct _finddata_t* data) {
  _find_search_t* fs = (_find_search_t*)id;

  /* Loop until we run out of entries or find the next one */
  dirent* entry;

  for (;;) {
    if (!(entry = readdir(fs->dirfd))) return -1;

    /* See if the filename matches our pattern */
    if (fnmatch(fs->pattern, entry->d_name, 0) == 0) break;
  }

  if (fs->curfn) free(fs->curfn);

  data->name = fs->curfn = cppfd::Utils::StrDup(entry->d_name);

  size_t namelen = strlen(entry->d_name);
  char* xfn = new char[fs->dirlen + 1 + namelen + 1];
  sprintf(xfn, "%s/%s", fs->directory, entry->d_name);

  /* stat the file to get if it's a subdir and to find its length */
  struct stat stat_buf;

  if (stat(xfn, &stat_buf)) {
    // Hmm strange, imitate a zero-length file then
    data->attrib = _A_NORMAL;
    data->size = 0;
  } else {
    if (S_ISDIR(stat_buf.st_mode))
      data->attrib = _A_SUBDIR;
    else
      /* Default type to a normal file */
      data->attrib = _A_NORMAL;

    data->size = (unsigned long)stat_buf.st_size;
  }

  delete[] xfn;

  /* Files starting with a dot are hidden files in Unix */
  if (data->name[0] == '.') data->attrib |= _A_HIDDEN;

  return 0;
}

int _findclose(intptr_t id) {
  int ret;
  _find_search_t* fs = (_find_search_t*)id;

  ret = fs->dirfd ? closedir(fs->dirfd) : 0;
  free(fs->pattern);
  free(fs->directory);

  if (fs->curfn) free(fs->curfn);

  delete fs;

  return ret;
}

intptr_t _findfirst(const char* pattern, struct _finddata_t* data) {
  _find_search_t* fs = new _find_search_t;
  fs->curfn = NULL;
  fs->pattern = NULL;

  // Separate the mask from directory name
  const char* mask = strrchr(pattern, '/');

  if (mask) {
    fs->dirlen = mask - pattern;
    mask++;
    fs->directory = (char*)malloc(fs->dirlen + 1);
    memcpy(fs->directory, pattern, fs->dirlen);
    fs->directory[fs->dirlen] = 0;
  } else {
    mask = pattern;
    fs->directory = cppfd::Utils::StrDup(".");
    fs->dirlen = 1;
  }

  fs->dirfd = opendir(fs->directory);

  if (!fs->dirfd) {
    _findclose((intptr_t)fs);

    return -1;
  }

  /* Hack for "*.*" -> "*' from DOS/Windows */
  if (strcmp(mask, "*.*") == 0) mask += 2;

  fs->pattern = cppfd::Utils::StrDup(mask);

  /* Get the first entry */
  if (_findnext((intptr_t)fs, data) < 0) {
    _findclose((intptr_t)fs);

    return -1;
  }

  return (intptr_t)fs;
}
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
  strRes.clear();
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
  strRes.clear();
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

cppfd::String Utils::StringExcapeChar(const String& strInput, const char excapeChar) {
  String strRes;
  for (char ch : strInput) {
    if (ch == excapeChar) {
      strRes.append(1, excapeChar);
    }
    strRes.append(1, ch);
  }
  return std::move(strRes);
 }

cppfd::String Utils::StringUnExcapeChar(const String& strInput, const char excapeChar) {
  String strRes;
  for (size_t i = 0; i < strInput.size(); ++i) {
    if (strInput[i] == excapeChar && i + 1 < strInput.size() && strInput[i + 1] == excapeChar) {
      strRes.append(1, strInput[i]);
      i++;
    } else {
      strRes.append(1, strInput[i]);
    }
  }
  return std::move(strRes);
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

uint64_t Utils::GetCurrentTime() {
  timeval tv;
  if (GetTimeOfDay(&tv, nullptr) != 0) return 0;
  return ((uint64_t)tv.tv_usec) / 1000 + ((uint64_t)tv.tv_sec) * 1000;
}

void Utils::StringTrim(String& strInput, bool bLeft /*= true*/, bool bRight /*= true*/) {
  static const String strDelims = " \t\r\n ";

  if (bRight) strInput.erase(strInput.find_last_not_of(strDelims) + 1);  // trim bRight
  if (bLeft) strInput.erase(0, strInput.find_first_not_of(strDelims));  // trim bLeft
}

FORCEINLINE void MkDir(const char* szDir) {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  _mkdir(szDir);
#else
  mkdir(szDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

void Utils::MkDirs(const String& strPath) { 
  String strTmp = strPath; 
  StringTrim(strTmp);
  std::replace(strTmp.begin(), strTmp.end(), '\\', '/');
  if (strTmp == "." || strTmp == "./" || strTmp == ".." || strTmp == "../") {
    return;
  }
  size_t ipos = strTmp.find('/');
  size_t isize = strTmp.size();
  while (ipos != String::npos) {
    String strDir = strTmp.substr(0, ipos);
    if (strDir != "." && strDir != "..") MkDir(strDir.c_str());
    ipos = strTmp.find('/', ipos + 1);
  }
  MkDir(strTmp.c_str());
}

int Utils::GetProcessID() {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  return ::GetCurrentProcessId();
#else
  return getpid();
#endif
}

void Utils::FindFiles(const String& strPattern, bool bRecursive, std::vector<String>* pFileList) {
  if (pFileList == nullptr) return;
  String strTmpPattern = strPattern;
  if (strTmpPattern == "." || strTmpPattern == "..") {
    strTmpPattern += "/";
  }
  std::replace(strTmpPattern.begin(), strTmpPattern.end(), '\\', '/');
  size_t pos1 = strTmpPattern.rfind('/');

  String directory = "./";
  String pattern = strTmpPattern;

  if (pos1 != strTmpPattern.npos) {
    directory = strTmpPattern.substr(0, pos1 + 1);
    pattern = strTmpPattern.substr(pos1 + 1);
  }

  if (pattern == "") {
    pattern = "*.*";
    strTmpPattern = directory + pattern;
  }

  intptr_t hFile;
  _finddata_t fileinfo;

  if ((hFile = _findfirst(strTmpPattern.c_str(), &fileinfo)) != -1) {
    do {
      if (!(fileinfo.attrib & _A_SUBDIR)) {
        String strFile = directory;
        strFile += fileinfo.name;
        pFileList->push_back(strFile);
      }
    } while (_findnext(hFile, &fileinfo) == 0);

    _findclose(hFile);
  }

  if (!bRecursive) {
    return;
  }

  String strNewPattern = directory + "*.*";

  if ((hFile = _findfirst(strNewPattern.c_str(), &fileinfo)) != -1) {
    do {
      if ((fileinfo.attrib & _A_SUBDIR)) {
        if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) {
          String strSubDir = directory;
          strSubDir += fileinfo.name;
          strSubDir += "/";
          strSubDir += pattern;
          FindFiles(strSubDir, bRecursive, pFileList);
        }
      }
    } while (_findnext(hFile, &fileinfo) == 0);

    _findclose(hFile);
  }
}

 WeakPtrArray::WeakPtrArray(uint32_t uCapacity) : mCapacity(uCapacity), mCount(0) {
  if (mCapacity == 0) mCapacity = 1;
   mItems = new Item*[mCapacity];
   for (uint32_t i = 0; i < mCapacity; i++) {
     mItems[i] = nullptr;
   }
 }

 WeakPtrArray::~WeakPtrArray() { SAFE_DELETE_ARRAY(mItems);}

bool WeakPtrArray::Add(Item* pItem) {
  if (pItem == nullptr) return false;
  if (mCount >= mCapacity) {
    uint32_t uNewCap = mCapacity * 1.5;
    if (uNewCap < mCapacity + 10) uNewCap = mCapacity + 10;
    Resize(uNewCap);
  }
  if (pItem->mIdx != -1 || pItem->mArray != nullptr) return false;
  pItem->mIdx = mCount++;
  pItem->mArray = this;
  mItems[pItem->mIdx] = pItem;
  return true;
 }

bool WeakPtrArray::Del(Item* pItem) {
  if (pItem == nullptr || pItem->mArray != this) return false;
  if (pItem->mIdx < 0 || (uint32_t) pItem->mIdx >= mCount || mItems[pItem->mIdx] != pItem) {
    LOG_ERROR("array or item may error");
    return false;
  }
  int32_t nIdx = pItem->mIdx;
  mItems[nIdx] = mItems[mCount - 1];
  mItems[nIdx]->mIdx = nIdx;
  mItems[mCount - 1] = nullptr;
  mCount--;
  pItem->mArray = nullptr;
  return true;
 }

void WeakPtrArray::Clear() {
  for (uint32_t i = 0; i < mCount; i++) {
    mItems[i]->mIdx = -1;
    mItems[i]->mArray = nullptr;
    mItems[i] = nullptr;
  }
  mCount = 0;
}

void WeakPtrArray::Resize(uint32_t uNewCap) {
  if (uNewCap <= mCapacity) return;
  Item** pNewItems = new Item*[uNewCap];
  uint32_t i = 0;
  for (; i < mCapacity; i++) {
    pNewItems[i] = mItems[i];
  }
  for (; i < uNewCap; i++)
  {
    pNewItems[i] = nullptr;
  }
  SAFE_DELETE_ARRAY(mItems);
  mItems = pNewItems;
  mCapacity = uNewCap;
}

FileWatcher::FileWatcher(const char* szFileFullPath) {
  mFileFullPath = szFileFullPath;
  MyStat(mFileFullPath.c_str(), &mStatbuf);
}

FileWatcher::FileWatcher(const char* szFileName, const char* szPath) {
  mFileFullPath = "";
  if (szPath) {
    mFileFullPath = szPath;
  }
  mFileFullPath += szFileName;
  MyStat(mFileFullPath.c_str(), &mStatbuf);
}

FileWatcher::FileWatcher() {
  mFileFullPath = "";
  ::memset(&mStatbuf, 0, sizeof(mStatbuf));
}

bool FileWatcher::IsFileChanged() const {
  if (mFileFullPath == "") {
    return false;
  }
  struct stat tmpStat;
  if (MyStat(mFileFullPath.c_str(), &tmpStat) == false) {
    return false;
  }

  return tmpStat.st_mtime != mStatbuf.st_mtime || tmpStat.st_size != mStatbuf.st_size;
}

void FileWatcher::Update() { MyStat(mFileFullPath.c_str(), &mStatbuf); }

bool FileWatcher::Watch(const char* szFileFullPath) {
  if (MyStat(szFileFullPath, &mStatbuf)) {
    mFileFullPath = szFileFullPath;
    return true;
  }
  return false;
}

bool FileWatcher::MyStat(const char* szFileName, struct stat* buf) {
  if (szFileName == nullptr || szFileName[0] == 0) {
    LOG_ERROR("empty filename");
    return false;
  }
  int nRes = stat(szFileName, buf);
  if (nRes != 0) {
    switch (errno) {
      case ENOENT:
        // KLOG_WARNING("File %s not found.\n", szFileName);
        break;
      case EINVAL:
        LOG_WARNING("Invalid parameter to _stat, file:%s.\n", szFileName);
        break;
      default:
        LOG_WARNING("Unexpected error in _stat, file:%s.\n", szFileName);
        break;
    }
    return false;
  }
  return true;
}

}  // namespace cppfd