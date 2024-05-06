#pragma once
#include "Prerequisites.h"
#include "Queue.h"
#include "ThreadData.h"
#include<condition_variable>
#include<thread>

namespace cppfd {
class Thread;
class ThreadPool;

class ThreadEvent {
 public:
  ThreadEvent(bool bSignal = true) : mSignal(bSignal) {}
  void Wait(uint32_t uMillisec);
  void WaitInfinite();
  void Post();
 private:
  std::condition_variable mCond;
  std::mutex mMutex;
  bool mSignal;
};

#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
typedef DWORD TID;
typedef unsigned __int64 MODEL_PART;
#else
typedef pthread_t TID;
#endif

// 可重入的读写锁, 多少次lock就需要对应多少次unlock,  只能加读锁或者写锁，不能同时加两种锁
class ReadWriteLock {
 public:
  enum ELockType {
    None,
    ReadFirst,
    WriteFirst,
  };

 public:
  ReadWriteLock(ELockType lockType = ELockType::None);
  void ReadLock();
  void WriteLock();
  void UnLock();
  bool IsNoLock();

 private:
  std::mutex mMutex;
  std::condition_variable mReadCV;
  std::condition_variable mWriteCV;
  int32_t mReadWaitingCount;
  int32_t mWriteWaitingCount;
  ELockType mLockType;
  std::map<TID, int32_t> mReadingThreadLockTimes;
  TID mWriteThreadID;
  int32_t mWriteLockTimes;
};


class ThreadKeeper {
  friend class Thread;

 public:
  ~ThreadKeeper() { mRWLocker.UnLock(); }
  Thread* Keep();
  FORCEINLINE void Release() { mRWLocker.UnLock(); }

 private:
  Thread* mThread;
  ReadWriteLock mRWLocker;
  bool mIsLock;
};

class Thread {
  friend class ThreadPool;
 public:
  Thread();
  virtual ~Thread();
  virtual void Start(const String& strName);
  virtual bool Run(const String& strName);
  virtual void Stop();

  FORCEINLINE static Thread* GetCurrentThread() { return (Thread*)sThreadData.Get(); }
  FORCEINLINE static std::shared_ptr<ThreadKeeper> GetCurrentThreadKeeper() {
    Thread* pCurrThread = (Thread*)sThreadData.Get();
    if (pCurrThread) return pCurrThread->mKeeper;
    return nullptr;
  }
  FORCEINLINE static TID GetCurrentThreadID() {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
  }
  static void Milisleep(uint32_t uMiliSec);

  int32_t RunTask();
  void Post(const std::function<void()>& func);

  FORCEINLINE void Dispatch(const std::function<void()>& func) {
    if (!func) return;
    if (GetCurrentThread() == this) {
      func();
      return;
    }
    Post(func);
  }

  template<typename R>
  R Invoke(const std::function<R()>& func) {
    Thread* pCurrThread = GetCurrentThread();
    if (pCurrThread == this || pCurrThread == nullptr) return func();
    R res;
    ThreadEvent ev(false);
    Post([&]() {
      res = func();
      ev.Post();
    });
    ev.WaitInfinite();
    return res;
  }

  void Invoke(const std::function<void()>& func) {
    Thread* pCurrThread = GetCurrentThread();
    if (pCurrThread == this || pCurrThread == nullptr) {
      func();
      return;
    }
    ThreadEvent ev(false);
    Post([&]() {
      func();
      ev.Post();
    });
    ev.WaitInfinite();
  }

  void Async(const std::function<void()>& func, const std::function<void()>& cbFunc);

  FORCEINLINE const String& GetName() const { return mName; }

 private:
  static void Routine(Thread* pThread) noexcept;
 private:
  std::atomic_int mRunningFlag;
  String mName;
  std::thread mThread;
  ThreadEvent mEvent;
  static ThreadData sThreadData;
  TMultiV1Queue<std::function<void()> > mTaskQueue;
  std::shared_ptr<ThreadKeeper> mKeeper;
  ThreadPool* mPool;
};

class ThreadPool {
  friend class Thread;
 public:
  ThreadPool(int32_t nThreadNum);
  virtual ~ThreadPool();

  virtual void Start(const String& strName);
  virtual void Stop();

  FORCEINLINE void Post(const std::function<void()>& func) {
    if (!func) return;
    mTaskQueue.Enqueue(func);
    mQueueReady.notify_one();
  }

  FORCEINLINE void Dispatch(const std::function<void()>& func) {
    if (!func) return;
    Thread* pCurrThread = Thread::GetCurrentThread();
    if (pCurrThread != nullptr && pCurrThread->mPool == this) {
      func();
      return;
    }
    Post(func);
  }

  template <typename R>
  R Invoke(const std::function<R()>& func) {
    Thread* pCurrThread = GetCurrentThread();
    if (pCurrThread == nullptr || pCurrThread->mPool == this) return func();
    R res;
    ThreadEvent ev(false);
    Post([&]() {
      res = func();
      ev.Post();
    });
    ev.WaitInfinite();
    return res;
  }

  void Invoke(const std::function<void()>& func) {
    Thread* pCurrThread = Thread::GetCurrentThread();
    if (pCurrThread == nullptr ||  pCurrThread->mPool == this ) {
      func();
      return;
    }
    ThreadEvent ev(false);
    Post([&]() {
      func();
      ev.Post();
    });
    ev.WaitInfinite();
  }

  void Async(const std::function<void()>& func, const std::function<void()>& cbFunc);

  void BroadCast(const std::function<void()>& func);
  
  int32_t RunTask();
 private:
  Thread* mThreads;
  TQueue<std::function<void()> > mTaskQueue;
  std::mutex mQueueLock;               /* 队列互斥锁 */
  std::condition_variable mQueueReady; /* 队列条件锁 */
  String mName;
  int32_t mThreadNum;
  std::atomic_int mRunningFlag;
  std::atomic_int32_t mBroadCastNum;
};

}