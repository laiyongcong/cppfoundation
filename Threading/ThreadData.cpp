#include "ThreadData.h"
#include "Utils.h"
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
#  include <windows.h>
#endif

namespace cppfd {
class ThreadDataError : public std::runtime_error {
 public:
  ThreadDataError(const String& what) throw() : runtime_error(what.c_str()) { Utils::ExeceptionLog(what); }
};

ThreadData::ThreadData(ThreadDataDestructorFunc func) : mDestructor(func) {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  mKey = TlsAlloc();
  if (mKey == TLS_OUT_OF_INDEXES) throw ThreadDataError("TlsAlloc error out of indexes");
#else 
  int nRet = pthread_key_create(&mKey, func);
  if (nRet != 0) throw ThreadDataError("pthread_key_create error, ret:" + std::to_string(nRet));
#endif
}

ThreadData::~ThreadData() {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  if (mDestructor) {
    for (size_t i = 0, isize = mDataPtrList.size(); i < isize; i++) {
      void* lpData = mDataPtrList[i];
      if (lpData == nullptr) continue;
      mDestructor(lpData);
    }
    mDataPtrList.clear();
  }
  int nRet = TlsFree(mKey);
  nRet = (nRet != 0 ? 0 : GetLastError());
  if (nRet != 0) Utils::ExeceptionLog("TlsFree error:" + std::to_string(nRet));
#else
  int nRet = pthread_key_delete(mKey);
  if (nRet != 0) Utils::ExeceptionLog("pthread_key_delete error, ret:" + std::to_string(nRet));
#endif
}

bool ThreadData::Set(void* pData) {
  void* pOldData = Get();
  if (pData == pOldData) return true;
  if (pOldData != nullptr && mDestructor != nullptr) {
    mDestructor(pOldData);
  }
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  if (mDestructor && pData != nullptr) {
    std::lock_guard<std::mutex> lock(mLocker);
    mDataPtrList.push_back(pData);
  }
  int nRet = TlsSetValue(mKey, pData);
  nRet = (nRet != 0) ? 0 : GetLastError();
  return nRet == 0;
#else
  return pthread_setspecific(mKey, pData) == 0;
#endif
}

void* ThreadData::Get() {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  void* pData = TlsGetValue(mKey);
  if (pData == nullptr && GetLastError() != ERROR_SUCCESS) {
    return nullptr;
  }
  return pData;
#else
  return pthread_getspecific(mKey);
#endif
}

}  // namespace cppfd