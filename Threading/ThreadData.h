#pragma once
#include "Prerequisites.h"
#include <mutex>
#if CPPFD_PLATFORM != CPPFD_PLATFORM_WIN32
#include <pthread.h>
#endif

namespace cppfd {
#if CPPFD_PLATFORM != CPPFD_PLATFORM_WIN32
typedef pthread_key_t thread_key;
#else
typedef uint64_t thread_key;
#endif
class ThreadData {
  typedef void (*ThreadDataDestructorFunc)(void*);
 public:
  ThreadData(ThreadDataDestructorFunc func);
  ~ThreadData();

  bool Set(void* pData);
  void* Get();
 private:
  thread_key mKey;
  ThreadDataDestructorFunc mDestructor;
  std::mutex mLocker;
  std::vector<void*> mDataPtrList;
};
}