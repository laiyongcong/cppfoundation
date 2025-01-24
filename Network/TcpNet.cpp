#include "TcpNet.h"
#include "Log.h"
#include "Thread.h"

namespace cppfd {
struct PackImp : public Pack {
  char* mBuff;
  BaseNetDecoder* mDecoder;
  uint32_t mSize;
  uint32_t mRWOffset;
  bool mIsCrypto;
  std::shared_ptr<Pack> mOuterPack;
  PackImp(uint32_t uSize, BaseNetDecoder* pDecoder) : mSize(uSize + 1), mRWOffset(0), mIsCrypto(false) {
    mBuff = new char[mSize];
    mBuff[mSize - 1] = 0;
    mDecoder = pDecoder;
  }
  PackImp(std::shared_ptr<Pack> pPack, BaseNetDecoder* pDecoder) : mBuff(nullptr), mDecoder(pDecoder), mSize(0), mRWOffset(0), mIsCrypto(false), mOuterPack(pPack) {
    if (pPack == nullptr) return;
    mBuff = const_cast<char*>(pPack->GetBuff());
    mSize = pPack->GetDataLen() + 1;
  }
  ~PackImp() { if(mOuterPack == nullptr) SAFE_DELETE_ARRAY(mBuff); }

  virtual const char* GetBuff() { return mBuff; }
  virtual uint32_t GetDataLen() const {
    if (mDecoder == nullptr) return 0;
    uint32_t uSize = mDecoder->GetHeaderSize() + mDecoder->GetBodyLen(mBuff);
    if (uSize >= mSize) {
      LOG_ERROR("Decode size:%u larger than pack size:%u", uSize, mSize - 1);
      return 0;
    }
    return uSize;
  }
};

struct NetIOInfo {
  TMultiV1Queue<std::shared_ptr<PackImp> > mSendBuff;
  std::shared_ptr<PackImp> mRecvingHeader;
  std::shared_ptr<PackImp> mRecvingPack;
  uint32_t mEpFlag;
  uint64_t mSendCryptoSeed;
  uint64_t mRecvCryptoSeed;

  BaseNetDecoder* mDecoder;
  NetCryptoFunc mSendCryptoFunc;
  NetCryptoFunc mRecvCryptoFunc;
  
  NetIOInfo() : mDecoder(nullptr), mSendCryptoFunc(nullptr), mRecvCryptoFunc(nullptr), mSendCryptoSeed(0), mRecvCryptoSeed(0), mEpFlag(0) {}
};

class NetThread : public Thread {
 public:
  NetThread() : mEngine(nullptr), mListenFd(INVALID_SOCKET), mConnecters(MAX_EPOLL_EVENTS), mHeaderSize(0) { 
      mEpfd = epoll_create(MAX_EPOLL_EVENTS); 
  }
  ~NetThread() {
    Stop();
    close_epoll_fd(mEpfd);
    SocketAPI::Close(mListenFd);
    uint32_t uConnecterNum = mConnecters.Size();
    for (uint32_t i = 0; i < uConnecterNum; i++) {
      Connecter* pConn = (Connecter*)mConnecters.Get(i);
      if (pConn->mIsCreatedByClient) continue;
      delete pConn;
    }
    mConnecters.Clear();
  }
 public:
  bool Init(const String& strHost, int nPort) { 
    SockAddr sAddr;
    int nErrCode;
    String strErr;
    if (!sAddr.SetAddr(strHost.c_str(), nPort)) {
      LOG_FATAL("invalid addr %s:%d", strHost.c_str(), nPort);
      return false;
    }
    LOG_TRACE("resolve %s:%d to %s:%d", strHost.c_str(), nPort, sAddr.GetIpAddr(), sAddr.GetPort());
    mListenFd = ::socket(sAddr.GetFamily(), SOCK_STREAM, 0);
    if (mListenFd < 0) {
      LOG_FATAL("create socket error");
      return false;
    }
    if (!SocketAPI::SetSockAddrReuse(mListenFd, true) || !SocketAPI::SetSockPortReuse(mListenFd, true) || !SocketAPI::SetNonBlocking(mListenFd, true)) {
      SocketAPI::GetSocketError(strErr, nErrCode);
      LOG_ERROR("set addrport reuse and nonblocking failed:%s code:%d", strErr.c_str(), nErrCode);
      SocketAPI::Close(mListenFd);
      return false;
    }

    uint32_t uLen;
    auto pAddr = sAddr.GetSockAddr(uLen);
    if (::bind(mListenFd, pAddr, (int)uLen) != 0) {
      SocketAPI::GetSocketError(strErr, nErrCode);
      LOG_ERROR("socket bind failed:%s code:%d", strErr.c_str(), nErrCode);
      SocketAPI::Close(mListenFd);
      return false;
    }
    if (::listen(mListenFd, 50) != 0) {
      SocketAPI::GetSocketError(strErr, nErrCode);
      LOG_ERROR("socket listen failed:%s code:%d", strErr.c_str(), nErrCode);
      SocketAPI::Close(mListenFd);
      return false;
    }
    epoll_event ev;
    ev.data.ptr = &(mListenFd);
    ev.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(mEpfd, EPOLL_CTL_ADD, mListenFd, &ev) != 0) {
      SocketAPI::GetSocketError(strErr, nErrCode);
      LOG_ERROR("socket bind failed:%s code:%d", strErr.c_str(), nErrCode);
      SocketAPI::Close(mListenFd);
      return false;
    }
    return true;
  }

  void Process() { 
    String strErr;
    int nErrCode;
    uint64_t uTime = Utils::GetCurrentTime();
    RunTask();
    int nWaitTime = 1;
    if (mConnecters.Size() == 0) nWaitTime = 5;
    if (mConnecters.Size() == 0 && mListenFd == INVALID_SOCKET) {
      if (Utils::GetCurrentTime() == uTime) Thread::Milisleep(1);
      return;
    }
    int nFds = epoll_wait(mEpfd, mEvents, sizeof(mEvents) / sizeof(epoll_event), nWaitTime);
    if (nFds < 0) {
      ESockErrorType eErrType = SocketAPI::GetSocketError(strErr, nErrCode);
      if (eErrType == ESockErrorType_Eintr) {
        return;
      }
      LOG_ERROR("epoll wait error:%s code:%d", strErr.c_str(), nErrCode);
      return;
    }
    for (int i = 0; i < nFds; i++) {
      epoll_event& ev = mEvents[i];
      if ((SOCKET*)ev.data.ptr == &mListenFd) {
        AcceptNewConn();
        continue;
      }
      Connecter* pConn = (Connecter*)ev.data.ptr;
      if (pConn == nullptr) {
        LOG_ERROR("event connector is null");
        return;
      }
      if (ev.events & EPOLLOUT) {
        epoll_event newEv;
        newEv.data.ptr = pConn;
        newEv.events = EPOLLIN | EPOLLRDHUP | EPOLLPRI | EPOLLHUP | EPOLLERR | EPOLLET;
        pConn->mIO->mEpFlag = EPOLLIN | EPOLLRDHUP | EPOLLPRI | EPOLLHUP | EPOLLERR;
        epoll_ctl(mEpfd, EPOLL_CTL_MOD, pConn->mSock, &newEv);  // 取消EPOLLOUT， 否则可能没有数据的情况下一直可写
        
        if (!ProcessOutput(pConn)) {
          ProcessNetError(pConn);
          continue;
        }
      }
      if (ev.events & EPOLLIN || ev.events & EPOLLPRI) {
        if (!ProcessInput(pConn)) {
          ProcessNetError(pConn);
          continue;
        }
      }
      if (ev.events & EPOLLERR || ev.events & EPOLLHUP) {
        LOG_WARNING("epoll err, %s", pConn->Info().c_str());
        ProcessInput(pConn);
        ProcessNetError(pConn, "epollerr or epollhup");
        continue;
      }

      if (ev.events & EPOLLRDHUP) {
        LOG_WARNING("%s remote end closed or half-closed the connection", pConn->Info().c_str());
        ProcessInput(pConn);
        ProcessNetError(pConn, "EPOLLRDHUP");
        continue;
      }
    }
  }

  void AcceptNewConn() {
    struct sockaddr_storage addr;
    String strErr;
    int nErrCode;
    char ipStr[INET6_ADDRSTRLEN];
    while (1) {
      socklen_t addrLen = sizeof(addr);
      SOCKET sock = ::accept(mListenFd, (struct sockaddr*)&addr, &addrLen);
      if (((int)sock) < 0) {
        ESockErrorType eErrType = SocketAPI::GetSocketError(strErr, nErrCode);
        if (eErrType == ESockErrorType_block) return;
        if (eErrType == ESockErrorType_Eintr) continue;
        LOG_ERROR("accept error:%s code:%d", strErr.c_str(), nErrCode);
        return;
      }
      if (!SocketAPI::SetNonBlocking(sock, true)) {
        SocketAPI::GetSocketError(strErr, nErrCode);
        LOG_ERROR("SetNonBlocking error:%s code:%d", strErr.c_str(), nErrCode);
        SocketAPI::Close(sock);
        continue;
      }

      Connecter* pConn = mEngine->AllocateConnecter();
      if (pConn == nullptr) {
        LOG_ERROR("allocate connecter error");
        SocketAPI::Close(sock);
        return;
      }
      pConn->mSock = sock;

      if (addr.ss_family == AF_INET) {
        inet_ntop(AF_INET, &(((struct sockaddr_in*)&addr)->sin_addr), ipStr, sizeof(ipStr));
        pConn->mPeerPort = ntohs(((struct sockaddr_in*)&addr)->sin_port);
      } else {
        inet_ntop(AF_INET6, &(((struct sockaddr_in6*)&addr)->sin6_addr), ipStr, sizeof(ipStr));
        pConn->mPeerPort = ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
      }
      LOG_DEBUG("%s accepted", pConn->Info().c_str());
      pConn->mPeerIp = ipStr;
      pConn->mIO->mDecoder = mEngine->mDecoder;
      pConn->mIO->mSendCryptoFunc = mEngine->mSendCryptoFunc;
      pConn->mIO->mSendCryptoSeed = mEngine->mSendCryptoSeed;
      pConn->mIO->mRecvCryptoFunc = mEngine->mRecvCryptoFunc;
      pConn->mIO->mRecvCryptoSeed = mEngine->mRecvCryptoSeed;

      //windows 的accept完全在第一个线程发生， linux操作系统尝试分配负载，但是内核不知道应用的情况，负载容易不均衡
      pConn->mNetThread = mEngine->AllocateNetThread();  
      pConn->mNetThread->AddConnecter(pConn);
    }
  }

  void AddConnecter(Connecter* pConn) {
    Dispatch([=]() {
      pConn->mIO->mEpFlag = EPOLLIN | EPOLLRDHUP | EPOLLPRI | EPOLLHUP | EPOLLERR;
      pConn->mIO->mRecvingHeader.reset(new PackImp(mEngine->mDecoder->GetHeaderSize(), mEngine->mDecoder));
      epoll_event ev;
      ev.data.ptr = pConn;
      ev.events = EPOLLIN | EPOLLRDHUP | EPOLLPRI | EPOLLHUP | EPOLLERR | EPOLLET;
      epoll_ctl(mEpfd, EPOLL_CTL_ADD, pConn->mSock, &ev);
      mConnecters.Add(pConn);
      mEngine->OnConnecterCreate(pConn);
    });
  }

  void AddClientConnecter(std::shared_ptr<Connecter> pConn) {
    Dispatch([=]() {
      if (pConn.get() == nullptr) return;
      mClientConnecters[pConn->GetConnecterID()] = pConn;
    });
  }

  void ProcessNetError(Connecter* pConn, const String& strUserErr = "") {
    if (!mConnecters.IsContain(pConn)) return;
    mConnecters.Del(pConn);
    std::shared_ptr<Connecter> connPtr(pConn, [=](Connecter* ptr) {  if(!ptr->mIsCreatedByClient) delete ptr; });//让connecter在正确的时机被delete
    mClientConnecters.erase(pConn->GetConnecterID());
    epoll_ctl(mEpfd, EPOLL_CTL_DEL, pConn->mSock, NULL);
    mEngine->OnConnecterClose(connPtr, strUserErr);
    connPtr->mIsValid = false;
  }

  bool ProcessOutput(Connecter* pConn) { 
    NetIOInfo* pIO = pConn->mIO;
    String strErr;
    int nErrCode;
    while (!pIO->mSendBuff.IsEmpty()) {
      std::shared_ptr<PackImp>* pPack = pIO->mSendBuff.Peek();
      PackImp* pImpPack = pPack->get();
      if (pImpPack == nullptr) {
        LOG_ERROR("Invalid pack");
        pIO->mSendBuff.Pop();
        continue;
      }
      uint32_t uDataLen = pImpPack->GetDataLen();
      uint32_t& uOffset = pImpPack->mRWOffset;
      char* pBuff = pImpPack->mBuff;
      
      if (uOffset == 0 && pImpPack->mIsCrypto == false && pIO->mSendCryptoFunc != nullptr) {
        for (uint32_t i = 0; i < uDataLen; i++) {
          pBuff[i] ^= pIO->mSendCryptoFunc(pIO->mSendCryptoSeed);
        }
        pImpPack->mIsCrypto = true;
      }
      while (pImpPack->mRWOffset < uDataLen) {
        uint32_t uNeedSend = uDataLen - pImpPack->mRWOffset;
        int nSend = ::send(pConn->mSock, pBuff + uOffset, (int)uNeedSend, 0);
        if (nSend <= 0) {
          ESockErrorType eErrType = SocketAPI::GetSocketError(strErr, nErrCode);
          if (eErrType == ESockErrorType_Eintr) continue;
          if (eErrType == ESockErrorType_block) {
            uint32_t uOldFlag = pIO->mEpFlag;
            pIO->mEpFlag |= EPOLLOUT;
            epoll_event ev;
            ev.data.ptr = pConn;
            ev.events = pIO->mEpFlag | EPOLLET;
            if (uOldFlag != pIO->mEpFlag) epoll_ctl(mEpfd, EPOLL_CTL_MOD, pConn->mSock, &ev);
            return true;
          }
          LOG_ERROR("Send to %s error:%s code:%d", pConn->Info().c_str(), strErr.c_str(), nErrCode);
          return false;
        }
        pImpPack->mRWOffset += (uint32_t)nSend;
      }
      if (pImpPack->mRWOffset == uDataLen) pIO->mSendBuff.Pop();
    }
    return true;
  }

  bool ProcessInput(Connecter* pConn) { 
    NetIOInfo* pIO = pConn->mIO;
    String strErr;
    int nErrCode;

    std::shared_ptr<PackImp>& pHeader = pIO->mRecvingHeader;
    std::shared_ptr<PackImp>& pPack = pIO->mRecvingPack;
    while (true) {
      char* pHeaderBuf = pHeader->mBuff;
      uint32_t& uHeaderOffset = pHeader->mRWOffset;
      while (uHeaderOffset < mHeaderSize) {
        int nNeedRead = (int)(mHeaderSize - uHeaderOffset);
        int nRecv = ::recv(pConn->mSock, pHeaderBuf + uHeaderOffset, nNeedRead, 0);
        if (nRecv <= 0) {
          ESockErrorType eErrType = SocketAPI::GetSocketError(strErr, nErrCode);
          if (eErrType == ESockErrorType_Eintr) continue;
          if (eErrType == ESockErrorType_block) {
            return true;
          }
          LOG_ERROR("%s recv error:%s code:%d", pConn->Info().c_str(), strErr.c_str(), nErrCode);
          return false;
        }
        if (pIO->mRecvCryptoFunc) {
          for (int i = 0; i < nRecv; i++) {
            pHeaderBuf[uHeaderOffset + i] ^= pIO->mRecvCryptoFunc(pIO->mRecvCryptoSeed);
          }
        }
        uHeaderOffset += (uint32_t)nRecv;
        if (uHeaderOffset == mHeaderSize) {
          uint32_t uBodyLen = mEngine->mDecoder->GetBodyLen(pHeaderBuf);
          if (uBodyLen + mHeaderSize > MAX_PACK_LEN) {
            LOG_ERROR("%s invalid bodylen:%u", pConn->Info().c_str(), uBodyLen);
            return false;
          }
          pPack.reset(new PackImp(uBodyLen, mEngine->mDecoder));
        }
      }

      char* pPackBuff = pPack->mBuff;
      uint32_t& uOffset = pPack->mRWOffset;
      uint32_t uDataLen = mEngine->mDecoder->GetBodyLen(pHeaderBuf);
      while (uOffset < uDataLen) {
        int nNeedRead = (int)(uDataLen - uOffset);
        int nRecv = ::recv(pConn->mSock, pPackBuff + uOffset, nNeedRead, 0);
        if (nRecv <= 0) {
          ESockErrorType eErrType = SocketAPI::GetSocketError(strErr, nErrCode);
          if (eErrType == ESockErrorType_Eintr) continue;
          if (eErrType == ESockErrorType_block) {
            return true;
          }
          LOG_ERROR("%s recv error:%s code:%d", pConn->Info().c_str(), strErr.c_str(), nErrCode);
          return false;
        }
        if (pIO->mRecvCryptoFunc) {
          for (int i = 0; i < nRecv; i++) {
            pPackBuff[uOffset + i] ^= pIO->mRecvCryptoFunc(pIO->mRecvCryptoSeed);
          }
        }
        uOffset += (uint32_t)nRecv;
      }

      pHeader->mRWOffset = 0;
      nErrCode = mEngine->OnRecvMsg(pConn, pHeaderBuf, pPackBuff, uDataLen);
      pPack.reset();
      if (nErrCode != 0) pConn->Kick("Pack execute ret:" + std::to_string(nErrCode)); 
    }
    return true;
  }

  FORCEINLINE void ProcessSend(Connecter* pConn) {
    Dispatch([=]() {
      if (!mConnecters.IsContain(pConn)) return;
      if (!ProcessOutput(pConn)) ProcessNetError(pConn);
    });
  }

  FORCEINLINE void Kick(Connecter* pConn, const String& strMsg) {
    Dispatch([=]() {
      if (!mConnecters.IsContain(pConn)) return;
      ProcessNetError(pConn, strMsg);
    });
  }

  FORCEINLINE TcpEngine* GetEngine() const { return mEngine;}

 public:
  epoll_fd mEpfd;
  epoll_event mEvents[MAX_EPOLL_EVENTS];
  TcpEngine* mEngine;
  SOCKET mListenFd;
  WeakPtrArray mConnecters;
  uint32_t mHeaderSize;
  std::map<uint32_t, std::shared_ptr<Connecter>> mClientConnecters;
};

 Connecter::Connecter() : mNetThread(nullptr), mSock(INVALID_SOCKET), mPeerPort(-1), mIsCreatedByClient(false), mIsValid(true) { 
   static std::atomic_uint sConnecterID(0);
   mConnecterID = sConnecterID++;
   mIO = new NetIOInfo;
 }

 Connecter::~Connecter() {
   SocketAPI::Close(mSock);
   SAFE_DELETE(mIO);
 }

 bool Connecter::Send(const String& strCmd, const char* szPack, uint32_t uPackLen) {
  if (mNetThread == nullptr) {
    LOG_FATAL("no net thread");
    return false;
  }
  if (mIO == nullptr || mIO->mDecoder == nullptr) {
    LOG_FATAL("no decoder");
    return false;
  }
  BaseNetDecoder* pDecoder = mIO->mDecoder;

  std::shared_ptr<PackImp> pPack = std::make_shared<PackImp>(pDecoder->GetHeaderSize() + uPackLen, pDecoder);
  pDecoder->SetCmd(pPack->mBuff, strCmd);
  if (!pDecoder->SetBodyLen(pPack->mBuff, uPackLen)) {
    LOG_ERROR("Invalid PackLen:%u", uPackLen);
    return false;
  }
  if (szPack != nullptr && uPackLen > 0) ::memcpy(pPack->mBuff + pDecoder->GetHeaderSize(), szPack, uPackLen);
  mIO->mSendBuff.Enqueue(pPack);

  mNetThread->ProcessSend(this);
  return true;
}


bool Connecter::Send(std::shared_ptr<Pack> pPack) {
  if (mNetThread == nullptr) {
    LOG_FATAL("no net thread");
    return false;
  }
  if (mIO == nullptr || mIO->mDecoder == nullptr) {
    LOG_FATAL("no decoder");
    return false;
  }
  BaseNetDecoder* pDecoder = mIO->mDecoder;
  if (pPack == nullptr || pDecoder->GetHeaderSize() + pDecoder->GetBodyLen(pPack->GetBuff()) != pPack->GetDataLen()) {
    LOG_ERROR("Invalid Pack");
    return false;
  }
  std::shared_ptr<PackImp> pPackImp = std::make_shared<PackImp>(pPack, pDecoder);
  mIO->mSendBuff.Enqueue(pPackImp);
  mNetThread->ProcessSend(this);
  return true;
}

bool Connecter::IsValid() const {
  if (mSock == INVALID_SOCKET) return false;
  if (mNetThread == nullptr) return false;
  return mIsValid;
}

void Connecter::Kick(const String& strMsg) {
  if (mNetThread == nullptr) {
    LOG_ERROR("no net thread");
    return;
  }
  mNetThread->Kick(this, strMsg);
}

TcpEngine::TcpEngine(uint32_t uNetThreadNum, BaseNetDecoder* pDecoder, const std::type_info& tMsgClass, int nPort, const String& strHost /*= "0.0.0.0"*/)
    : mHost(strHost), mPort(nPort), mNetThreadNum(uNetThreadNum), mDecoder(pDecoder), mSendCryptoSeed(0), mRecvCryptoSeed(0), mSendCryptoFunc(nullptr), mRecvCryptoFunc(nullptr), mHeaderSize(0) {
  mNetThreads = new NetThread[uNetThreadNum];
  for (uint32_t i = 0; i < uNetThreadNum; i++) {
    mNetThreads[i].mEngine = this;
  }
  mMsgClass = Class::GetClass(tMsgClass);
  if (mMsgClass == nullptr) {
    LOG_FATAL("unknow msg class:%s", Demangle(tMsgClass.name()).c_str());
  }
  if (mDecoder) {
    mHeaderSize = mDecoder->GetHeaderSize();
  }
 }

 TcpEngine::~TcpEngine() {
   SAFE_DELETE_ARRAY(mNetThreads);
 }

void TcpEngine::SetCrypto(NetCryptoFunc SendCryptoFunc, NetCryptoFunc RecvCryptoFunc, uint64_t uSendCrypto, uint64_t uRecvCrypto) { 
  if (mNetThreadNum == 0) return;
  if (mNetThreads[0].IsRunning()) {
    LOG_ERROR("Engine is running, set crypto failed");
    return;
  }
  mSendCryptoSeed = uSendCrypto;
  mRecvCryptoSeed = uRecvCrypto;
  mSendCryptoFunc = SendCryptoFunc;
  mRecvCryptoFunc = RecvCryptoFunc;
}

bool TcpEngine::Start(const String& strNetThreadName /*= ""*/) {
  if (mMsgClass == nullptr) {
    LOG_FATAL("unknow msg class");
    return false;
  }
  if (mDecoder == nullptr) {
    LOG_FATAL("unknow Decoder");
    return false;
  }
  for (uint32_t i = 0; i < mNetThreadNum; i++) {
    mNetThreads[i].mHeaderSize = mHeaderSize;
    mNetThreads[i].Start(strNetThreadName + "Thread" + std::to_string(i));
    if (mPort > 0 && !mNetThreads[i].Invoke<bool>([=]() { return mNetThreads[i].Init(mHost, mPort); })) {
      LOG_FATAL("init net failed");
      return false;
    }
  }

  for (uint32_t i = 0; i < mNetThreadNum; i++) {
    mNetThreads[i].Post([=](){
      while (mNetThreads[i].IsRunning()){
        mNetThreads[i].Process();
      }
    });
  }

  return true;
}

void TcpEngine::OnConnecterCreate(Connecter* pConn) {
  if (pConn == nullptr) return;
  LOG_TRACE("%s create", pConn->Info().c_str());
}

void TcpEngine::OnConnecterClose(std::shared_ptr<Connecter> pConn, const String& szErrMsg) {
  if (pConn.get() == nullptr) return;
  LOG_TRACE("%s disconect, msg:%s", pConn->Info().c_str(), szErrMsg.c_str());
}

int TcpEngine::OnRecvMsg(Connecter* pConn, const char* szHeaderBuff, const char* szBodybuff, uint32_t uBodyLen) {
  const String& strCmd = mDecoder->GetCmd(szHeaderBuff);
  const StaticMethod* pMethod = mMsgClass->GetStaticMethod(strCmd.c_str());
  if (pMethod == nullptr) {
    LOG_FATAL("unknow msg:%s", strCmd.c_str());
    return -1;
  }
  return pMethod->Invoke<int, Connecter*, const char*, uint32_t>(pConn, szBodybuff, uBodyLen);
}


cppfd::NetThread* TcpEngine::AllocateNetThread() {
  uint32_t uMin = UINT_MAX;
  uint32_t nIdx = UINT_MAX;
  for (uint32_t i = 0; i < mNetThreadNum; i++) {
    if (mNetThreads[i].mConnecters.Size() < uMin) {
      uMin = mNetThreads[i].mConnecters.Size();
      nIdx = i;
    }
  }
  if (nIdx == UINT_MAX) {
    LOG_ERROR("Allocate NetThreadFailed");
    return nullptr;
  }

  return &(mNetThreads[nIdx]);
}

std::shared_ptr<Connecter> TcpEngine::Connect(const char* szHost, int nPort, int* pMicroTimeout, bool bLingerOn /*= true*/, uint32_t uLinger /*= 0*/, int nClientPort /* = 0*/) {
  NetThread* pNetThread = AllocateNetThread();
  if (pNetThread == nullptr) {
    return nullptr;
  }

  if (!mNetThreads[0].IsRunning()) {
    LOG_ERROR("Engine is not runing");
    return nullptr;
  }

  SOCKET sock = INVALID_SOCKET;
  if (!TcpClient::Connect(sock, szHost, nPort, pMicroTimeout, bLingerOn, uLinger, nClientPort)) return nullptr;

   Connecter* pConn = AllocateConnecter();
  if (pConn == nullptr) {
    LOG_ERROR("allocate connecter error");
    SocketAPI::Close(sock);
    return nullptr;
  }
  pConn->mSock = sock;
  pConn->mNetThread = pNetThread;
  SocketAPI::GetPeerIpPort(sock, pConn->mPeerIp, pConn->mPeerPort);

  pConn->mIO->mDecoder = mDecoder;
  pConn->mIO->mSendCryptoFunc = mSendCryptoFunc;
  pConn->mIO->mSendCryptoSeed = mSendCryptoSeed;
  pConn->mIO->mRecvCryptoFunc = mRecvCryptoFunc;
  pConn->mIO->mRecvCryptoSeed = mRecvCryptoSeed;

  pNetThread->AddConnecter(pConn);
  pConn->mIsCreatedByClient = true;
  auto pResConn = std::shared_ptr<Connecter>(pConn);
  pNetThread->AddClientConnecter(pResConn);
  return pResConn;
}

bool TcpClient::Connect(SOCKET& sock, const char* szHost, int nPort, int* pMicroTimeout, bool bLingerOn /* = true*/, uint32_t uLinger /*= 0*/, int nClientPort /*= 0*/) {
  String strErr;
  int nErrCode, nRet;
  fd_set readSet, writeSet;
  struct sockaddr* pSockAddr = nullptr;
  uint32_t nSockAddrLen = 0;
  struct timeval selectTime;
  int nOldTimeout = 0;
  if (pMicroTimeout == nullptr) {
    LOG_ERROR("invalid param pMicroTimeout is null");
    return false;
  }
  if (*pMicroTimeout < 0) *pMicroTimeout = 0;
  nOldTimeout = *pMicroTimeout;

  SockAddr sockAddr;
  if (!sockAddr.SetAddr(szHost, nPort)) return false;
  if (sock != INVALID_SOCKET) SocketAPI::Close(sock);
  if ((sock = socket(sockAddr.GetFamily(), SOCK_STREAM, 0)) < 0) {
    LOG_ERROR("create sock failed while connecting to %s:%d", szHost, nPort);
    return false;
  }
  pSockAddr = sockAddr.GetSockAddr(nSockAddrLen);

  if (nClientPort > 0) {
    if (SocketAPI::SetSockAddrReuse(sock, true) == false || SocketAPI::SetSockPortReuse(sock, true) == false) {
      SocketAPI::GetSocketError(strErr, nErrCode);
      LOG_ERROR("SetSocket addr or port reuse failed:%s, errno:%d", strErr.c_str(), nErrCode);
      SocketAPI::Close(sock);
      return false;
    }
    if (sockAddr.GetFamily() == AF_INET) {
      struct sockaddr_in ClientSockAddr;
      ClientSockAddr.sin_family = AF_INET;
      ClientSockAddr.sin_addr.s_addr = INADDR_ANY;
      ClientSockAddr.sin_port = htons(nClientPort);
      if (::bind(sock, (const sockaddr*)&ClientSockAddr, (int)sizeof(ClientSockAddr)) != 0) {
        SocketAPI::GetSocketError(strErr, nErrCode);
        LOG_ERROR("bind socket to local port:%d failed:%s, errno:%d", nClientPort, strErr.c_str(), nErrCode);
        SocketAPI::Close(sock);
        return false;
      }
    } else {
      struct sockaddr_in6 ClientSockAddr;
      ClientSockAddr.sin6_family = AF_INET6;
      ClientSockAddr.sin6_addr = in6addr_any;
      ClientSockAddr.sin6_port = htons(nClientPort);
      if (::bind(sock, (const sockaddr*)&ClientSockAddr, (int)sizeof(ClientSockAddr)) != 0) {
        SocketAPI::GetSocketError(strErr, nErrCode);
        LOG_ERROR("bind socket to local port:%d failed:%s, errno:%d", nClientPort, strErr.c_str(), nErrCode);
        SocketAPI::Close(sock);
        return false;
      }
    }
  }
  if (!SocketAPI::SetLinger(sock, bLingerOn, uLinger)) {  // 打开，并设置linger为0，可避免timewait
    SocketAPI::GetSocketError(strErr, nErrCode);
    LOG_ERROR("SetLinger failed:%s, errno:%d", strErr.c_str(), nErrCode);
    SocketAPI::Close(sock);
    return false;
  }
  if (!SocketAPI::SetNonBlocking(sock, true)) {
    SocketAPI::GetSocketError(strErr, nErrCode);
    LOG_ERROR("SetNonBlocking failed:%s, errno:%d", strErr.c_str(), nErrCode);
    SocketAPI::Close(sock);
    return false;
  }
  nRet = ::connect(sock, pSockAddr, (int)nSockAddrLen);
  if (nRet != 0) {
    ESockErrorType eErrType = SocketAPI::GetSocketError(strErr, nErrCode);
    if (eErrType == ESockErrorType_normal) {
      LOG_ERROR("connect to %s:%d failed:%s, errno:%d", szHost, nPort, strErr.c_str(), nErrCode);
      SocketAPI::Close(sock);
      return false;
    }

    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    writeSet = readSet;
    selectTime.tv_sec = *pMicroTimeout / 1000000, selectTime.tv_usec = *pMicroTimeout % 1000000;
    nRet = select(sock + 1, &readSet, &writeSet, NULL, &selectTime);
    if (selectTime.tv_sec * 1000000 + selectTime.tv_usec < 10) selectTime.tv_sec = 0, selectTime.tv_usec = 0;
    *pMicroTimeout = selectTime.tv_sec * 1000000 + selectTime.tv_usec;
    if (nRet == 0) {
      LOG_ERROR("timeout(%d us) while connecting to (%s:%d)", nOldTimeout, szHost, nPort);
      SocketAPI::Close(sock);
      return false;
    }
    if (FD_ISSET(sock, &readSet) || FD_ISSET(sock, &writeSet)) {
      if (SocketAPI::IsSocketError(sock)) {
        LOG_ERROR("socket error while connecting to %s:%d", szHost, nPort);
        SocketAPI::Close(sock);
        return false;
      }
    } else {
      LOG_ERROR("socket select error while connecting to %s:%d", szHost, nPort);
      SocketAPI::Close(sock);
      return false;
    }
  }
  return true;
}

int TcpClient::Read(SOCKET sock, void* szBuf, int len, int* pMicroTimeout) {
  String strErr;
  int nErrCode;
  int nRet;
  int nLeft, nRead;
  char* pPtr;
  struct timeval selectTime;
  fd_set readSet;
  int nOldTimeout;

  if (sock == INVALID_SOCKET) {
    LOG_ERROR("socket is valid");
    return false;
  }

  if (pMicroTimeout == nullptr || szBuf == nullptr) {
    LOG_ERROR("invalid param pMicroTimeout or szBuf is null");
    return false;
  }
  if (*pMicroTimeout < 0) *pMicroTimeout = 0;
  nOldTimeout = *pMicroTimeout;

  nLeft = len;
  nRead = 0;
  pPtr = (char*)szBuf;
  selectTime.tv_sec = *pMicroTimeout / 1000000, selectTime.tv_usec = *pMicroTimeout % 1000000;
  while (nLeft > 0) {
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    nRet = select(sock + 1, &readSet, NULL, NULL, &selectTime);
    if (selectTime.tv_sec * 1000000 + selectTime.tv_usec < 10) selectTime.tv_sec = 0, selectTime.tv_usec = 0;
    if (nRet < 0) {
      LOG_ERROR("select fail while read from %d", (int)sock);
      return -1;
    } else if (nRet == 0) {
      LOG_DEBUG("timeout(%d us) while read from %d", nOldTimeout, (int)sock);
      break;
    }
    nRead = recv(sock, pPtr, nLeft, 0);
    if (nRead < 0) {
      ESockErrorType eErrType = SocketAPI::GetSocketError(strErr, nErrCode);
      if (eErrType == ESockErrorType_normal) {
        SocketAPI::Close(sock);
        LOG_ERROR("read fail from %d, errno:%d, err:%s", (int)sock, nErrCode, strErr.c_str());
        return -1;
      }
      continue;
    } else if (nRead == 0) {
      SocketAPI::GetSocketError(strErr, nErrCode);
      LOG_ERROR("read fail from %d(disconnect), err:%s, errno:%d", (int)sock, strErr.c_str(), nErrCode);
      SocketAPI::Close(sock);
      return -1;
    }
    pPtr += nRead;
    nLeft -= nRead;
  }
  *pMicroTimeout = selectTime.tv_sec * 1000000 + selectTime.tv_usec;
  return len - nLeft;
}

int TcpClient::Write(SOCKET sock, void* szBuf, int len, int* pMicroTimeout) {
  String strErr;
  int nErrCode;
  int nRet;
  int nLeft, nWrite;
  char* pPtr;
  struct timeval selectTime;
  fd_set writeSet;
  int nOldTimeout;

  if (sock == INVALID_SOCKET) {
    LOG_ERROR("socket is valid");
    return false;
  }

  if (pMicroTimeout == nullptr || szBuf == nullptr) {
    LOG_ERROR("invalid param pMicroTimeout or szBuf is null");
    return false;
  }
  if (*pMicroTimeout < 0) *pMicroTimeout = 0;
  nOldTimeout = *pMicroTimeout;

  nLeft = len;
  nWrite = 0;
  pPtr = (char*)szBuf;
  selectTime.tv_sec = *pMicroTimeout / 1000000, selectTime.tv_usec = *pMicroTimeout % 1000000;
  while (nLeft > 0) {
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);
    nRet = select(sock + 1, NULL, &writeSet, NULL, &selectTime);
    if (selectTime.tv_sec * 1000000 + selectTime.tv_usec < 10) selectTime.tv_sec = 0, selectTime.tv_usec = 0;
    if (nRet < 0) {
      LOG_ERROR("select fail while write to %d", (int)sock);
      return -1;
    } else if (nRet == 0) {
      LOG_DEBUG("timeout(%d us) while write to %d", nOldTimeout, (int)sock);
      break;
    }
    nWrite = send(sock, pPtr, nLeft, 0);
    if (nWrite < 0) {
      ESockErrorType eErrType = SocketAPI::GetSocketError(strErr, nErrCode);
      if (eErrType == ESockErrorType_normal) {
        SocketAPI::Close(sock);
        LOG_ERROR("write failed to %d, errno:%d, err:%s", (int)sock, nErrCode, strErr.c_str());
        return -1;
      }
      continue;
    } else if (nWrite == 0) {
      SocketAPI::GetSocketError(strErr, nErrCode);
      LOG_ERROR("write fail to %d(disconnect), err:%s, errno:%d", (int)sock, strErr.c_str(), nErrCode);
      SocketAPI::Close(sock);
      return -1;
    }
    pPtr += nWrite;
    nLeft -= nWrite;
  }
  *pMicroTimeout = selectTime.tv_sec * 1000000 + selectTime.tv_usec;
  return len - nLeft;
}

uint8_t DefaultNetCryptoFunc(uint64_t& ulSeed) {
  uint32_t x = (uint32_t)(ulSeed >> 32);
  uint32_t y = (uint32_t)(ulSeed & 0xFFFFFFFF);

  x = 99069 * x + 12345;
  y ^= (y << 13);
  y ^= (y >> 17);
  y ^= (y << 5);

  ulSeed = x;
  ulSeed = (ulSeed << 32) | y;
  return (x + y) % 256;
}

}  // namespace cppfd