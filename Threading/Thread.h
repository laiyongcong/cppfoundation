#pragma once
#include "Prerequisites.h"
#include "Queue.h"
#include "ThreadData.h"
#include <shared_mutex>
#include<condition_variable>
#include<thread>

namespace cppfd {
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
class Thread;
class ThreadKeeper {
  friend class Thread;
 public:
  ~ThreadKeeper() { mRWLocker.unlock_shared(); }
  Thread* Keep();
  FORCEINLINE void Release() { mRWLocker.unlock_shared(); }
 private:
  Thread* mThread;
  std::shared_mutex mRWLocker;
};

typedef TQueue<std::function<void()> > TaskQueue;
class Thread {
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
  static void Milisleep(uint32_t uMiliSec);

  void RunTask();
  FORCEINLINE void Post(const std::function<void()>& func) {
    if (!func) return;
    mTaskQueue.Enqueue(func);
    mEvent.Post();
  }

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

 private:
  static void Routine(Thread* pThread) noexcept;
 private:
  std::atomic_int mRunningFlag;
  String mName;
  std::thread mThread;
  ThreadEvent mEvent;
  static ThreadData sThreadData;
  TaskQueue mTaskQueue;
  std::shared_ptr<ThreadKeeper> mKeeper;
};

}