#include "Thread.h"
#if CPPFD_PLATFORM != CPPFD_PLATFORM_WIN32
#  include <arpa/inet.h>
#  include <errno.h>
#  include <netdb.h>
#  include <sys/select.h>
#  include <sys/time.h>
#else
#  include <MMSystem.h>
#  include <WinSock2.h>
#  include <process.h>
#  pragma comment(lib, "winmm")
#endif

namespace cppfd {

void ThreadTaskQueueDestroy(void* pTaskQueue) {
  if (pTaskQueue == nullptr) return;
  delete (TaskQueue*)pTaskQueue;
}

void ThreadEvent::Wait(uint32_t uMillisec) {
  std::unique_lock<std::mutex> lck(mMutex);
  if (!mSignal) mCond.wait_for(lck, std::chrono::milliseconds(uMillisec));
  mSignal = false;
}

void ThreadEvent::WaitInfinite() {
  std::unique_lock<std::mutex> lck(mMutex);
  if (!mSignal) mCond.wait(lck);
  mSignal = false;
}

void ThreadEvent::Post() {
  std::unique_lock<std::mutex> lck(mMutex);
  mSignal = true;
  mCond.notify_one();
}

 Thread::Thread() : mRunningFlag(0) { 
  mKeeper = std::make_shared<ThreadKeeper>();
  mKeeper->mThread = this;
 }

 Thread::~Thread() {}

void Thread::Start(const String& strName) {
  if (++mRunningFlag > 1) {
    --mRunningFlag;
    return;
  }
  mName = strName;
  mThread = std::thread(Routine, this);
 }

bool Thread::Run(const String& strName) {
  if (++mRunningFlag > 1) {
    --mRunningFlag;
    return false;
  }
  Routine(this);
  --mRunningFlag;
  return true;
 }

void Thread::Stop() {
  if (++mRunningFlag > 1) {
    --mRunningFlag;
    --mRunningFlag;
  } else {
    return;
  }
  mEvent.Post();
  if (GetCurrentThread() != this && mThread.joinable()) mThread.join();
 }

void Thread::Milisleep(uint32_t uMiliSec) {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  timeBeginPeriod(1);
  Sleep(uMiliSec);
  timeEndPeriod(1);
#else
  struct timeval tv;
  tv.tv_sec = uMiliSec / 1000;
  tv.tv_usec = (uMiliSec % 1000) * 1000;
  select(1, NULL, NULL, NULL, &tv);
#endif
  //std::this_thread::sleep_for(std::chrono::milliseconds(uMiliSec));
 }

 void Thread::RunTask() {
    while (true) {
      std::function<void()> func;
      if (!mTaskQueue.Dequeue(func)) break;
      func();
    }
}

void Thread::Async(const std::function<void()>& func, const std::function<void()>& cbFunc) {
    std::shared_ptr<ThreadKeeper> pKeeper = GetCurrentThreadKeeper();
    if (pKeeper.get() == nullptr) {
      Invoke(func);
      cbFunc();
      return;
    }
    Post([=]() { 
      func();
      Thread* pCallThread = pKeeper->Keep();
      if (pCallThread != nullptr) pCallThread->Post(cbFunc);
      pKeeper->Release();
    });
}

void Thread::Routine(Thread* pThread) noexcept {
  if (pThread == nullptr) return;
  sThreadData.Set(pThread);
  srand((uint32_t)time(nullptr));
  while (pThread->mRunningFlag > 0) {
    pThread->RunTask();
    pThread->mEvent.WaitInfinite();
  }
  pThread->mKeeper->mRWLocker.lock();
  pThread->mKeeper->mThread = nullptr;
  pThread->mKeeper->mRWLocker.unlock();
 }

ThreadData Thread::sThreadData(nullptr);

}