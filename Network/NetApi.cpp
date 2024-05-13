#include "NetApi.h"
#include "Log.h"


namespace cppfd {
NetDecoder g_NetHeaderDecoder;
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

cppfd::ESockErrorType SocketAPI::GetSocketError(String& strErrMsg, int& nErrno) {
  nErrno = NetLastError(strErrMsg);
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  if (nErrno == WSAEWOULDBLOCK || nErrno == WSAEINPROGRESS) return ESockErrorType_block;
#else
  if (nErrno == EWOULDBLOCK || nErrno == EAGAIN || nErrno == EINPROGRESS) return ESockErrorType_block;
  if (nErrno == EINTR) return ESockErrorType_Eintr;
#endif
  return ESockErrorType_normal;
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
#endif

int close_epoll_fd(epoll_fd& fd) {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
  SAFE_DELETE(fd);
  return 0;
#else
  return close(fd);
#endif
}

