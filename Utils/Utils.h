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
  static String StringExcapeChar(const String& strInput, const char excapeChar);
  static String StringUnExcapeChar(const String& strInput, const char excapeChar);
  static String Double2String(double dNumber, uint32_t uDigit);
  static int GetTimeOfDay(struct timeval* tp, void* tzp);
  static uint64_t GetCurrentTime();
  static void StringTrim(String& strInput, bool bLeft = true, bool bRight = true);
  static void MkDirs(const String& strPath);
  static int GetProcessID();
  static void FindFiles(const String& strPattern, bool bRecursive, std::vector<String>* pFileList);
  FORCEINLINE static char* StrDup(const char* szInput) {
    char* szRes = nullptr;
    if (szInput && (szRes = (char*)malloc(strlen(szInput) + 1))) strcpy(szRes, szInput);
    return szRes;
  }
};

class WeakPtrArray {
 public:
  class Item {
    friend class WeakPtrArray;
   private:
    int32_t mIdx;
    WeakPtrArray* mArray;
   public:
    Item() : mIdx(-1), mArray(nullptr) {}
    virtual ~Item(){}
  };

 public:
  WeakPtrArray(uint32_t uCapacity);
  ~WeakPtrArray();

  bool Add(Item* pItem);
  bool Del(Item* pItem);
  FORCEINLINE Item* Get(uint32_t idx) {
    if (idx >= mCount) return nullptr;
    return mItems[idx];
  }
  void Clear();
  FORCEINLINE uint32_t Size() const { return mCount; }

 private:
  void Resize(uint32_t uNewCap);
 private:
  Item** mItems;
  uint32_t mCapacity;
  std::atomic_uint mCount;
};

class FileWatcher {
 public:
  FileWatcher();
  FileWatcher(const char* szFileFullPath);
  FileWatcher(const char* szFileName, const char* szPath);

  bool IsFileChanged() const;
  void Update();
  const char* GetFile() const { return mFileFullPath.c_str(); }
  bool Watch(const char* szFileFullPath);

  static bool MyStat(const char* szFileName, struct stat* buf);

 private:
  struct stat mStatbuf;
  String mFileFullPath;
};
}