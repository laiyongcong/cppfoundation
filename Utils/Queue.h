#pragma once
#include "Utils.h"

namespace cppfd {
template<typename T>
class TQueue : NonCopyable {
 public:
  using ElementType = T;
  TQueue() : mSize(0) { 
    TNodeInfo tmpNode;
    tmpNode.mNode = new TNode;
    mHead = mTail = tmpNode;
  }
  ~TQueue() {
    TNode* pTail = mTail.load().mNode;
    while (pTail != nullptr) {
      TNode* pNode = pTail;
      pTail = pNode->NextNode;
      delete pNode;
    }
  }
  bool Dequeue(ElementType& OutItem) { 
    TNodeInfo currTail, nextVerTail, nextTail;
    uint32_t uCurrentVersion = mVersion++;
    do {
      do {
        currTail = mTail;
        if (currTail.mNode->NextNode == nullptr) return false;
        nextVerTail = currTail;
        nextVerTail.mVersion = uCurrentVersion;
      } while (!std::atomic_compare_exchange_weak(&mTail, &currTail, nextVerTail));
      
      currTail = nextVerTail;
      nextTail.mNode = currTail.mNode->NextNode;
    } while (!std::atomic_compare_exchange_weak(&mTail, &currTail, nextTail));

    TNode* pPop = currTail.mNode->NextNode;
    OutItem = std::move(pPop->Item);
    pPop->Item = ElementType();
    delete currTail.mNode;
    mSize--;
    return true;
  }

  bool Enqueue(const ElementType& InItem) { 
    TNode* pNewNode = new TNode(InItem);
    if (pNewNode == nullptr) return false;
    TNodeInfo currHead, nextVerHead, nextHead;
    nextHead.mNode = pNewNode;
    uint32_t uCurrentVersion = mVersion++;
    do {
      do 
      {
        currHead = mHead;
        nextVerHead = currHead;
        nextVerHead.mVersion = uCurrentVersion;
      } while (!std::atomic_compare_exchange_weak(&mHead, &currHead, nextVerHead));
      currHead = nextVerHead;
    } while (!std::atomic_compare_exchange_weak(&mHead, &currHead, nextHead));
    std::atomic_thread_fence(std::memory_order_seq_cst);
    currHead.mNode->NextNode = nextHead.mNode;
    mSize++;
    return true;
  }

  FORCEINLINE bool IsEmpty() const { return mSize == 0; }

  // not support multi thread comsumers
  ElementType* Peek() {
    if (mTail.load().mNode->NextNode == nullptr) return nullptr;
    return &(mTail.load().mNode->NextNode->Item);
  }

  // not support multi thread comsumers
  FORCEINLINE const ElementType* Peek() const { return const_cast<TQueue*>(this)->Peek(); }

  bool Pop() { 
    TNodeInfo currTail, nextVerTail, nextTail;
    uint32_t uCurrentVersion = mVersion++;
    do {
      do {
        currTail = mTail;
        if (currTail.mNode->NextNode == nullptr) return false;
        nextVerTail = currTail;
        nextVerTail.mVersion = uCurrentVersion;
      } while (!std::atomic_compare_exchange_weak(&mTail, &currTail, nextVerTail));

      currTail = nextVerTail;
      nextTail.mNode = currTail.mNode->NextNode;
    } while (!std::atomic_compare_exchange_weak(&mTail, &currTail, nextTail));
    delete currTail.mNode;
    mSize--;
    return true;
  }

  void Empty() { while (Pop()); }

  uint64_t GetSize() const { return mSize; }

 private:
  struct TNode {
    TNode* volatile NextNode;
    ElementType Item;
    TNode() : NextNode(nullptr) {}
    explicit TNode(const ElementType& InItem) : NextNode(nullptr), Item(InItem) {}
    explicit TNode(ElementType& InItem) : NextNode(nullptr), Item(std::move(InItem)) {}
  };
  std::atomic_uint mVersion;

  struct TNodeInfo {
    TNode* mNode;
    uint32_t mVersion;
    TNodeInfo() : mNode(nullptr), mVersion(0){}
  };

  std::atomic<TNodeInfo> mHead;
  std::atomic<TNodeInfo> mTail;

  std::atomic_uint64_t mSize;
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