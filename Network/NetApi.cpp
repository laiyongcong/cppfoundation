#include "NetApi.h"
#include "Log.h"


namespace cppfd {

int NetLastError(String& strErrMsg) {
  int nErrno = 0;
  char errMsg[1024] = {0};
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  nErrno = WSAGetLastError();
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, nErrno, 0, errMsg, sizeof(errMsg) - 1, NULL);
#else
  nErrno = errno;
  safe_printf(errMsg, sizeof(errMsg), "%s", strerror(errno));
#endif
  strErrMsg = errMsg;
  return nErrno;
}

ESockErrorType GetSocketError(String& strErrMsg, int& nErrno) {
  String strErr;
  nErrno = NetLastError(strErr);
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  if (nErrno == WSAEWOULDBLOCK || nErrno == WSAEINPROGRESS) return ESockErrorType_block;
#else
  if (nErrno == EWOULDBLOCK || nErrno == EAGAIN || nErrno == EINPROGRESS) return ESockErrorType_block;
  if (nErrno == EINTR) return ESockErrorType_Eintr;
#endif
  return ESockErrorType_normal;
}

static std::atomic_int g_SockInitFlag(0);
SockInitor::SockInitor() {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  if (++g_SockInitFlag > 1) {
    --g_SockInitFlag;
    return;
  }
  WORD wVersionRequested;
  WSADATA wsaData;
  wVersionRequested = MAKEWORD(2, 2);

  if (WSAStartup(wVersionRequested, &wsaData) != 0) {
    WSACleanup();
  }
#endif
}

 SockInitor::~SockInitor() {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  if (--g_SockInitFlag < 0) {
    ++g_SockInitFlag;
    return;
  }
  WSACleanup();
#endif
 }

bool SockAddr::SetAddr(const char* szAddr, int nPort) {
  if (szAddr == nullptr || szAddr[0] == 0) {
    LOG_ERROR("invalid socket addr");
    return false;
  }
  mPort = nPort;
  struct addrinfo Hints, *pRes = nullptr;
  memset(&Hints, 0, sizeof(Hints));
  int nRet = 0;
  if ((nRet = getaddrinfo(szAddr, nullptr, &Hints, &pRes)) != 0) {
    LOG_ERROR("getaddrinfo:%s error:%s", szAddr, gai_strerror(nRet));
    return false;
  }
  if (pRes == nullptr) {
    LOG_ERROR("invalid socket addr:%s", szAddr);
    return false;
  }
  void* addr;
  if (pRes->ai_family == AF_INET) {
    mIpv4 = *(struct sockaddr_in*)pRes->ai_addr;
    addr = &(mIpv4.sin_addr);
    mIpv4.sin_port = htons(nPort);
    mFamily = AF_INET;
  } else {
    mIpv6 = *(struct sockaddr_in6*)pRes->ai_addr;
    addr = &(mIpv6.sin6_addr);
    mIpv6.sin6_port = htons(nPort);
    mFamily = AF_INET6;
  }

  inet_ntop(pRes->ai_family, addr, mAddr, sizeof(mAddr));
  freeaddrinfo(pRes);
  return true;
 }

sockaddr* SockAddr::GetSockAddr(uint32_t& uLen) {
  if (mFamily == AF_INET) {
    uLen = (uint32_t)sizeof(mIpv4);
    return (sockaddr*)&mIpv4;
  } else if (mFamily == AF_INET6) {
    uLen = (uint32_t)sizeof(mIpv6);
    return (sockaddr*)&mIpv6;
  }
  uLen = 0;
  return nullptr;
 }

bool SocketAPI::GetLocalIPPort(SOCKET sock, String& strIp, int& nPort) {
  if (sock == INVALID_SOCKET) return false;
  char ipStr[INET6_ADDRSTRLEN];

  struct sockaddr_storage addr;
  socklen_t addrLen = sizeof(addr);
  if (getsockname(sock, (struct sockaddr*)&addr, &addrLen) != 0) {
    String strErr;
    int nErrCode = NetLastError(strErr);
    LOG_ERROR("sock:%d error:%s errcode:%d", (int)sock, strErr.c_str(), nErrCode);
    return false;
  }

  if (addr.ss_family == AF_INET) {
    inet_ntop(AF_INET, &(((struct sockaddr_in*)&addr)->sin_addr), ipStr, sizeof(ipStr));
    nPort = ntohs(((struct sockaddr_in*)&addr)->sin_port);
  } else {
    inet_ntop(AF_INET6, &(((struct sockaddr_in6*)&addr)->sin6_addr), ipStr, sizeof(ipStr));
    nPort = ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
  }
  strIp = ipStr;
  return true;
 }

 bool SocketAPI::GetPeerIpPort(SOCKET sock, String& strIp, int& nPort) {
  if (sock == INVALID_SOCKET) return false;
  char ipStr[INET6_ADDRSTRLEN];

  struct sockaddr_storage addr;
  socklen_t addrLen = sizeof(addr);
  if (getpeername(sock, (struct sockaddr*)&addr, &addrLen) != 0) {
    String strErr;
    int nErrCode = NetLastError(strErr);
    LOG_ERROR("sock:%d error:%s errcode:%d", (int)sock, strErr.c_str(), nErrCode);
    return false;
  }

  if (addr.ss_family == AF_INET) {
    inet_ntop(AF_INET, &(((struct sockaddr_in*)&addr)->sin_addr), ipStr, sizeof(ipStr));
    nPort = ntohs(((struct sockaddr_in*)&addr)->sin_port);
  } else {
    inet_ntop(AF_INET6, &(((struct sockaddr_in6*)&addr)->sin6_addr), ipStr, sizeof(ipStr));
    nPort = ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
  }
  strIp = ipStr;
  return true;
 }

bool TcpClient::Connect(SOCKET& sock, const char* szHost, int nPort, int* pMicroTimeout, uint32_t uLinger /*= 0*/, int nClientPort /*= 0*/) {
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
    if (SocketAPI::SetSockAddrReuse(sock, true) == false) {
      nErrCode = NetLastError(strErr);
      LOG_ERROR("SetSocket addr reuse failed:%s, errno:%d", strErr.c_str(), nErrCode);
      SocketAPI::Close(sock);
      return false;
    }
#if CPPFD_PLATFORM != CPPFD_PLATFORM_WIN32
#  ifndef SO_REUSEPORT
    LOG_ERROR("Not Support SO_REUSEPORT!!!");
    return false;
#  else
    int nOn = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char*)&nOn, (int)sizeof(nOn)) != 0) {
      nErrCode = NetLastError(strErr);
      LOG_ERROR("SetSocket port reuse failed:%s, errno:%d", strErr.c_str(), nErrCode);
      SocketAPI::Close(sock);
      return false;
    }
#  endif
#endif
    if (sockAddr.GetFamily() == AF_INET) {
      struct sockaddr_in ClientSockAddr;
      ClientSockAddr.sin_family = AF_INET;
      ClientSockAddr.sin_addr.s_addr = INADDR_ANY;
      ClientSockAddr.sin_port = htons(nClientPort);
      if (::bind(sock, (const sockaddr*)&ClientSockAddr, (int)sizeof(ClientSockAddr)) != 0) {
        nErrCode = NetLastError(strErr);
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
        nErrCode = NetLastError(strErr);
        LOG_ERROR("bind socket to local port:%d failed:%s, errno:%d", nClientPort, strErr.c_str(), nErrCode);
        SocketAPI::Close(sock);
        return false;
      }
    }
  }
  if (!SocketAPI::SetLinger(sock, true, uLinger)) { //链接断开后立刻回收资源，从而避免timewait
    nErrCode = NetLastError(strErr);
    LOG_ERROR("SetLinger failed:%s, errno:%d", strErr.c_str(), nErrCode);
    SocketAPI::Close(sock);
    return false;
  }
  if (!SocketAPI::SetNonBlocking(sock, true)) {
    nErrCode = NetLastError(strErr);
    LOG_ERROR("SetNonBlocking failed:%s, errno:%d", strErr.c_str(), nErrCode);
    SocketAPI::Close(sock);
    return false;
  }
  nRet = ::connect(sock, pSockAddr, (int)nSockAddrLen);
  if (nRet != 0) {
    ESockErrorType eErrType = GetSocketError(strErr, nErrCode);
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
      break;
    } else if (nRet == 0) {
      LOG_WARNING("timeout(%d us) while read from %d", nOldTimeout, (int)sock);
      break;
    }
    nRead = recv(sock, pPtr, nLeft, 0);
    if (nRead < 0) {
      ESockErrorType eErrType = GetSocketError(strErr, nErrCode);
      if (eErrType == ESockErrorType_normal) {
        SocketAPI::Close(sock);
        LOG_ERROR("read fail from %d, errno:%d, err:%s", (int)sock, nErrCode, strErr.c_str());
        return -1;
      }
      continue;
    } else if (nRead == 0) {
      nErrCode = NetLastError(strErr);
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
      break;
    } else if (nRet == 0) {
      LOG_WARNING("timeout(%d us) while write to %d", nOldTimeout, (int)sock);
      break;
    }
    nWrite = send(sock, pPtr, nLeft, 0);
    if (nWrite < 0) {
      ESockErrorType eErrType = GetSocketError(strErr, nErrCode);
      if (eErrType == ESockErrorType_normal) {
        SocketAPI::Close(sock);
        LOG_ERROR("write failed to %d, errno:%d, err:%s", (int)sock, nErrCode, strErr.c_str());
        return -1;
      }
      continue;
    } else if (nWrite == 0) {
      nErrCode = NetLastError(strErr);
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

}  // namespace cppfd
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
epoll_fd epoll_create(int nsize) { return new (std::nothrow) _epoll_fd; }

int epoll_ctl(epoll_fd epfd, int op, SOCKET fd, epoll_event* event) {
  if (epfd == nullptr) {
    return -1;
  }

  epfd->mLock.lock();

  switch (op) {
    case EPOLL_CTL_ADD: {
      if (epfd->fdMap.find(fd) != epfd->fdMap.end()) {
        epfd->fdMap[fd] = *event;
        break;
      }

      FD_SET(fd, &(epfd->fdset));
      epfd->fdMap[fd] = *event;
    } break;

    case EPOLL_CTL_MOD: {
      if (epfd->fdMap.find(fd) != epfd->fdMap.end()) {
        epfd->fdMap[fd] = *event;
      }
    } break;

    case EPOLL_CTL_DEL: {
      if (epfd->fdMap.find(fd) == epfd->fdMap.end()) {
        break;
      }

      FD_CLR(fd, &(epfd->fdset));
      epfd->fdMap.erase(fd);
    } break;

    default:
      break;
  }

  epfd->mLock.unlock();
  return 0;
}

int epoll_wait(epoll_fd epfd, struct epoll_event* events, int maxevents, int timeout) {
  if (epfd == nullptr || events == nullptr || maxevents < (int)epfd->fdset.fd_count) {
    return -1;
  }

  struct timeval tv, *pTv = nullptr;
  if (timeout >= 0) {
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    pTv = &tv;
  }

  fd_set fdWrite;
  FD_ZERO(&fdWrite);
  fd_set fdOneShot;
  FD_ZERO(&fdOneShot);
  fd_set fdRead;
  FD_ZERO(&fdRead);

  fd_set* pWriteFDS = NULL;

  epfd->mLock.lock();
  fd_set fdExc = epfd->fdset;

  for (int i = 0; (unsigned int)i < epfd->fdset.fd_count; i++) {
    epoll_event& epev = epfd->fdMap[epfd->fdset.fd_array[i]];

    if (epev.events & EPOLLOUT) {
      FD_SET(epfd->fdset.fd_array[i], &fdWrite);
      pWriteFDS = &fdWrite;
    }

    if (epev.events & EPOLLIN) {
      FD_SET(epfd->fdset.fd_array[i], &fdRead);
    }

    if (epev.events & EPOLLONESHOT) {
      FD_SET(epfd->fdset.fd_array[i], &fdOneShot);
    }
  }

  epfd->mLock.unlock();

  int ret = select(0, &fdRead, pWriteFDS, &fdExc, pTv);

  if (ret == SOCKET_ERROR) {
    return -1;
  }

  epfd->mLock.lock();
  int nResCount = 0;

  for (int i = 0; i < (int)epfd->fdset.fd_count; i++) {
    epoll_data epdata = epfd->fdMap[epfd->fdset.fd_array[i]].data;
    events[nResCount].data = epdata;
    events[nResCount].events = 0;
    bool evFlag = false;

    if (FD_ISSET(epfd->fdset.fd_array[i], &fdRead)) {
      events[nResCount].events |= EPOLLIN;
      evFlag = true;
    }

    if (FD_ISSET(epfd->fdset.fd_array[i], &fdExc)) {
      events[nResCount].events |= EPOLLERR;
      evFlag = true;
    }

    if (FD_ISSET(epfd->fdset.fd_array[i], &fdWrite)) {
      events[nResCount].events |= EPOLLOUT;
      evFlag = true;
    }

    if (evFlag) {
      nResCount++;
    }
  }

  for (INT i = 0; i < (int)fdOneShot.fd_count; i++) {
    FD_CLR(fdOneShot.fd_array[i], &(epfd->fdset));
    epfd->fdMap.erase(fdOneShot.fd_array[i]);
  }

  epfd->mLock.unlock();

  return nResCount;
}

int close_epoll_fd(epoll_fd& fd) {
#  if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  SAFE_DELETE(fd);
  return 0;
#  else
  return close(fd);
#  endif
}
#endif

