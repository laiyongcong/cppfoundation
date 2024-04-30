#pragma once
#include "Utils.h"
#include <thread>
#include<condition_variable>
#include <mutex>

namespace cppfd {


template<typename T>
class TQueue : NonCopyable {
 public:
  using ElementType = T;
  TQueue(bool bMultiComsumerDelayDelete = false) : mSize(0), mValid(true), mDelayDelete(bMultiComsumerDelayDelete) { 
    mHead = mTail = mDelTail = new TNode;
    mDelTail->BeDeleted = true;
    if (mDelayDelete) {
      mDelThread = std::thread([=]() {
        while (mValid) {
          std::unique_lock<std::mutex> lck(mDelMutex);
          mDelReady.wait(lck, [=]() { return !mValid.load() && mDelTail->NextNode != nullptr && mDelTail->NextNode->BeDeleted.load(); });
          while (mDelTail->NextNode != nullptr && mDelTail->NextNode->BeDeleted) {
            TNode* pDel = mDelTail;
            mDelTail = mDelTail->NextNode;
            delete pDel;
          }
        }
      });
    }
  }
  ~TQueue() {
    if (mDelayDelete) {
      mValid = false;
      mDelReady.notify_one();
      mDelThread.join();
    }
    TNode* pTail = mTail.load();
    while (pTail != nullptr) {
      TNode* pNode = pTail;
      pTail = pNode->NextNode;
      delete pNode;
    }
  }
  bool Dequeue(ElementType& OutItem) { 
    TNode* pTail = mTail;
    do 
    {
      if (pTail->NextNode == nullptr) return false;
    } while (!std::atomic_compare_exchange_weak(&mTail, &pTail, pTail->NextNode));
    
    TNode* pPop = pTail->NextNode;
    OutItem = std::move(pPop->Item);
    pPop->BeDeleted = true;
    pPop->Item = ElementType();
    mSize--;
    if (mDelayDelete)
      mDelReady.notify_one();
    else
      delete pTail;
    return true;
  }

  bool Enqueue(const ElementType& InItem) { 
    TNode* pNewNode = new TNode(InItem);
    if (pNewNode == nullptr) return false;

    TNode* pCurrHead = mHead;
    while (!std::atomic_compare_exchange_weak(&mHead, &pCurrHead, pNewNode));
    std::atomic_thread_fence(std::memory_order_seq_cst);
    pCurrHead->NextNode = pNewNode;
    mSize++;
    return true;
  }

  FORCEINLINE bool IsEmpty() const { return mSize == 0; }

  // not support multi thread comsumers
  ElementType* Peek() {
    if (mTail.load()->NextNode == nullptr) return nullptr;
    return &(mTail.load()->NextNode->Item);
  }

  // not support multi thread comsumers
  FORCEINLINE const ElementType* Peek() const { return const_cast<TQueue*>(this)->Peek(); }

  bool Pop() { 
    TNode* pTail = mTail;
    do {
      if (pTail->NextNode == nullptr) return false;
    } while (!std::atomic_compare_exchange_weak(&mTail, &pTail, pTail->NextNode));
    TNode* pPop = pTail->NextNode;
    pPop->BeDeleted = true;
    pPop->Item = ElementType();
    mSize--;
    if (mDelayDelete)
      mDelReady.notify_one();
    else
      delete pTail;
    return true;
  }

  void Empty() { while (Pop()); }

  uint64_t GetSize() const { return mSize; }

 private:
  struct TNode {
    TNode* volatile NextNode;
    ElementType Item;
    std::atomic_bool BeDeleted;
    TNode() : NextNode(nullptr), BeDeleted(false){}
    explicit TNode(const ElementType& InItem) : NextNode(nullptr), Item(InItem), BeDeleted(false) {}
    explicit TNode(ElementType& InItem) : NextNode(nullptr), Item(std::move(InItem)), BeDeleted(false) {}
  };

  std::atomic<TNode*> mHead;
  std::atomic<TNode*> mTail;
  TNode* mDelTail;

  std::atomic_uint64_t mSize;

  std::condition_variable mDelReady;
  std::mutex mDelMutex;
  std::atomic_bool mValid;
  std::thread mDelThread;

  bool mDelayDelete;
};

template <typename T>
class T1V1Queue : NonCopyable {
 public:
  using ElementType = T;
  T1V1Queue() : mSize(0) {
    mHead = mTail = new TNode;
  }
  ~T1V1Queue() {
    while (mTail != nullptr) {
      TNode* pNode = mTail;
      mTail = pNode->NextNode;
      delete pNode;
    }
  }
  bool Dequeue(ElementType& OutItem) {
    TNode* pPop = mTail->NextNode;
    if (pPop == nullptr) return false;
    OutItem = std::move(pPop->Item);
    TNode* pOldTail = mTail;
    mTail = pPop;
    mTail->Item = ElementType();
    delete pOldTail;
    mSize--;
    return true;
  }

  bool Enqueue(const ElementType& InItem) {
    TNode* pNewNode = new TNode(InItem);
    if (pNewNode == nullptr) return false;
    TNode* OldHead = mHead;
    mHead = pNewNode;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    OldHead->NextNode = pNewNode;
    mSize++;
    return true;
  }

  FORCEINLINE bool IsEmpty() const { return mSize == 0; }

  ElementType* Peek() {
    if (mTail->NextNode == nullptr) return nullptr;
    return &(mTail->NextNode->Item);
  }

  FORCEINLINE const ElementType* Peek() const { return const_cast<T1V1Queue*>(this)->Peek(); }

  bool Pop() {
    TNode* pPop = mTail->NextNode;
    if (pPop == nullptr) return false;
    TNode* pOldTail = mTail;
    mTail = pPop;
    mTail->Item = ElementType();
    delete pOldTail;
    mSize--;
    return true;
  }

  void Empty() {
    while (Pop());
  }

  uint64_t GetSize() const { return mSize; }

 private:
  struct TNode {
    TNode* volatile NextNode;
    ElementType Item;
    TNode() : NextNode(nullptr) {}
    explicit TNode(const ElementType& InItem) : NextNode(nullptr), Item(InItem) {}
    explicit TNode(ElementType& InItem) : NextNode(nullptr), Item(std::move(InItem)) {}
  };

  alignas(16) TNode* volatile mHead;
  TNode* mTail;

  std::atomic_uint64_t mSize;
};
}