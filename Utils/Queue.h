#pragma once
#include "Utils.h"
#include <thread>
#include<condition_variable>
#include <mutex>

namespace cppfd {
template <typename T>
class T1V1Queue : NonCopyable {
 public:
  using ElementType = T;
  T1V1Queue() : mSize(0) { mHead = mTail = new TNode; }
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
    while (Pop())
      ;
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

template <typename T>
class TMultiV1Queue : NonCopyable {
 public:
  using ElementType = T;
  TMultiV1Queue() : mSize(0) { mHead = mTail = new TNode; }
  ~TMultiV1Queue() {
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

    TNode* pCurrHead = mHead;
    while (!std::atomic_compare_exchange_weak(&mHead, &pCurrHead, pNewNode))
      ;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    pCurrHead->NextNode = pNewNode;
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
    while (Pop())
      ;
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

  std::atomic<TNode*> mHead;
  TNode* mTail;

  std::atomic_uint64_t mSize;
};

template<typename T>
class TQueue : NonCopyable {
 public:
  using ElementType = T;
  TQueue() : mSize(0), mListSize(0), mUniqDelFlag(0) { 
    mHead = mTail = mDelTail = new TNode;
    mDelTail->BeDeleted = true;
  }
  ~TQueue() {
    TNode* pTail = mDelTail;
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
    uint64_t uCurrSize = --mSize;
    uint64_t uNeedDelNum = mListSize.load() - uCurrSize;
    if (uNeedDelNum > 256 && ++mUniqDelFlag == 1) {
      while (mDelTail->NextNode != nullptr && mDelTail->NextNode->BeDeleted && uNeedDelNum > 256) {
        TNode* pDel = mDelTail;
        mDelTail = mDelTail->NextNode;
        mListSize--;
        uNeedDelNum--;
        delete pDel;
      }
      --mUniqDelFlag;
    }
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
    mListSize++;
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
    uint64_t uCurrSize = --mSize;
    uint64_t uNeedDelNum = mListSize.load() - uCurrSize;
    if (uNeedDelNum > 256 && ++mUniqDelFlag == 1) {
      while (mDelTail->NextNode != nullptr && mDelTail->NextNode->BeDeleted && uNeedDelNum > 256) {
        TNode* pDel = mDelTail;
        mDelTail = mDelTail->NextNode;
        mListSize--;
        uNeedDelNum--;
        delete pDel;
      }
      --mUniqDelFlag;
    }
    return true;
  }

  void Empty() { while (Pop()); }

  uint64_t GetSize() const { return mSize; }

 private:
  void DeleteTail() { 
      uint64_t uNeedDelSize = mListSize - mSize;

  }

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
  std::atomic_uint64_t mListSize;
  std::atomic_int mUniqDelFlag;
};
}