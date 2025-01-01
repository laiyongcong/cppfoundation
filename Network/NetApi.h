#pragma once
#include "Prerequisites.h"
#include "Log.h"
/*
* 由于对windows下的完成端口不太熟悉，因此在windows下使用select对epoll接口进行模拟。
* 一般来说大家都不会使用windows系统作为服务器，所以问题不是太大，只是windows下编程会更方便一些
* 注意：epoll的边沿触发机制并没有模拟
*/

#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
#  include <WS2tcpip.h>
#  include <WinSock2.h>
#  pragma comment(lib, "Ws2_32.lib")
#  undef FD_SETSIZE
#  define FD_SETSIZE 4096
#  define MAX_EPOLL_EVENTS FD_SETSIZE

#  define EPOLLIN 1        // 表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
#  define EPOLLOUT 2       // 表示对应的文件描述符可以写；
#  define EPOLLPRI 4       // 表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
#  define EPOLLERR 8       // 表示对应的文件描述符发生错误；
#  define EPOLLHUP 16      // 表示对应的文件描述符被挂断；
#  define EPOLLET 32       // 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
#  define EPOLLONESHOT 64  // 只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
#  define EPOLLRDHUP 128

#  define EPOLL_CTL_ADD 1
#  define EPOLL_CTL_MOD 2
#  define EPOLL_CTL_DEL 3

typedef union epoll_data {
  void* ptr;
  SOCKET fd;
  unsigned int u32;
  unsigned long long u64;
} epoll_data_t;

struct epoll_event {
  unsigned int events; /* Epoll events */
  epoll_data_t data;   /* User data variable */
  epoll_event() {
    events = 0;
    data.ptr = nullptr;
  }
};

typedef struct _epoll_fd {
  fd_set fdset;
  std::mutex mLock;
  std::map<SOCKET, epoll_event> fdMap;
  _epoll_fd() { FD_ZERO(&fdset); }
  ~_epoll_fd() {}
}* epoll_fd;

extern epoll_fd epoll_create(int nsize);
extern int epoll_ctl(epoll_fd epfd, int op, SOCKET fd, epoll_event* event);
extern int epoll_wait(epoll_fd epfd, struct epoll_event* events, int maxevents, int timeout);

#else 
#  include <sys/socket.h>
#  include <unistd.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <sys/time.h>
#  include <fcntl.h>
#  include <errno.h>
#  include <sys/epoll.h>
#  define MAX_EPOLL_EVENTS 20000
typedef int epoll_fd;
typedef int SOCKET;
#  ifndef INVALID_SOCKET
#    define INVALID_SOCKET -1
#  endif  // !INVALID_SOCKET
#  define SOCKET_ERROR -1
#endif

extern int close_epoll_fd(epoll_fd& fd);

namespace cppfd {
enum ESockErrorType { 
	ESockErrorType_block = 0,
	ESockErrorType_Eintr,
	ESockErrorType_normal,
};

class SockInitor {
 public:
  SockInitor();
  ~SockInitor();
};

#define STR_ADDR_LEN (128)
class SockAddr {
 public:
  SockAddr() { ::memset(this, 0, sizeof(SockAddr)); }
  bool SetAddr(const char* szAddr, int nPort);
  sockaddr* GetSockAddr(uint32_t& uLen);
  FORCEINLINE int GetFamily() const { return mFamily; }
  FORCEINLINE const char* GetIpAddr() const { return mAddr; }
  FORCEINLINE int GetPort() const { return mPort; }
 private:
  int mFamily;
  char mAddr[STR_ADDR_LEN];
  int mPort;

  sockaddr_in mIpv4;
  sockaddr_in6 mIpv6;
};

class SocketAPI {
 public:
  FORCEINLINE static bool SetNonBlocking(SOCKET s, bool bOn) {
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
    ULONG argp = bOn ? 1 : 0;
    return ioctlsocket(s, FIONBIO, &argp) != SOCKET_ERROR;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (bOn)
      flags |= O_NONBLOCK;
    else
      flags &= ~O_NONBLOCK;

    fcntl(s, F_SETFL, flags);
    return true;
#endif
  }
  static ESockErrorType GetSocketError(String& strErrMsg, int& nErrno);
  FORCEINLINE static bool IsSocketError(SOCKET s) {
    int error;
    socklen_t len = (socklen_t)sizeof(error);
    return getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&error, &len) != 0;
  }
  FORCEINLINE static bool SetSendBuffSize(SOCKET s, int nSize) { return setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&nSize, (int)sizeof(nSize)) == 0; }
  FORCEINLINE static bool SetRecvBuffSize(SOCKET s, int nSize) { return setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&nSize, (int)sizeof(nSize)) == 0; }
  FORCEINLINE static int GetSendBuffSize(SOCKET s) { 
	int nSize = 0;
    socklen_t nLen = (socklen_t)sizeof(nSize);
    getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&nSize, &nLen);
    return nSize;
  }
  FORCEINLINE static int GetRecvBuffSize(SOCKET s) {
    int nSize = 0;
    socklen_t nLen = (socklen_t)sizeof(nSize);
    getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&nSize, &nLen);
    return nSize;
  }
  FORCEINLINE static bool SetTcpNoDelay(SOCKET s, bool bOn) { 
    int nOn = bOn ? 1 : 0;
    return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&nOn, (int)sizeof(nOn)) == 0;
  }
  FORCEINLINE static bool SetLinger(SOCKET s, bool bOn, uint32_t uLingerTime) {
    struct linger ling;
    ling.l_onoff = bOn ? 1 : 0;
    ling.l_linger = uLingerTime;
    return setsockopt(s, SOL_SOCKET, SO_LINGER, (char*)&ling, (int)sizeof(ling)) == 0;
  }

  FORCEINLINE static bool SetSockAddrReuse(SOCKET s, bool bOn) {
    int nOn = bOn ? 1 : 0;
    return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&nOn, (int)sizeof(nOn)) == 0;
  }

  FORCEINLINE static bool SetSockPortReuse(SOCKET s, bool bOn) {
#if CPPFD_PLATFORM != CPPFD_PLATFORM_WIN32
#ifndef SO_REUSEPORT
    return false;
#else
    int nOn = bOn ? 1 : 0;
    return setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char*)&nOn, (int)sizeof(nOn)) == 0;
#endif
#else
    return true;
#endif
  }

  FORCEINLINE static uint32_t GetLinger(SOCKET s) { 
    struct linger ling;
    socklen_t nLen = (socklen_t)sizeof(ling);
    getsockopt(s, SOL_SOCKET, SO_LINGER, (char*)&ling, &nLen);
    return ling.l_linger;
  }
  FORCEINLINE static int Close(SOCKET& s) {
    if (s == INVALID_SOCKET) return 0;
#if CPPFD_PLATFORM == CPPFD_PLATFORM_WIN32
    int ret = closesocket(s);
#else 
    int ret = close(s);
#endif
    s = INVALID_SOCKET;
    return ret;
  }

  static bool GetLocalIPPort(SOCKET sock, String& strIp, int& nPort);
  static bool GetPeerIpPort(SOCKET sock, String& strIp, int& nPort);
};

#pragma pack(push, 1)
struct NetHeader {
  char mCmd[32];  // 函数
  uint32_t mBodyLen;  // 包体长度,不包括包头
};
#pragma pack(pop)

//保留用户自定义包头的能力
class BaseNetDecoder {
 public:
  virtual ~BaseNetDecoder() {}
  virtual uint32_t GetBodyLen(const void* pHeader) = 0;                       // 获取头长度字段
  virtual bool SetBodyLen(void* pHeader, unsigned int bdlen) = 0;       // 设置body长度
  virtual uint32_t GetHeaderSize() = 0;                                 // 获取头长度
  virtual const String GetCmd(const void* pHeader) = 0;                      //获得包头的命令
  virtual bool SetCmd(void* pHeader, const String& strCmd) = 0;         //设置包头的命令
};

#define MAX_PACK_LEN (64 * 1024 * 1024)  // 64M
class NetDecoder : BaseNetDecoder {
 public:
  virtual uint32_t GetBodyLen(const void* pHeader) {
    if (pHeader == nullptr) return 0;
    return ntohl(((NetHeader*)pHeader)->mBodyLen);
  }
  virtual bool SetBodyLen(void* pHeader, uint32_t bdlen) {
    if (pHeader == nullptr || bdlen > MAX_PACK_LEN - sizeof(NetHeader)) return false;
    ((NetHeader*)pHeader)->mBodyLen = htonl(bdlen);
    return true;
  }
  virtual uint32_t GetHeaderSize() { return (uint32_t)sizeof(NetHeader); }
  virtual const String GetCmd(const void* pHeader) {
    if (pHeader == nullptr) return "";
    NetHeader* pH = (NetHeader*)pHeader;
    pH->mCmd[sizeof(pH->mCmd) - 1] = 0; //截断保护
    return pH->mCmd;
  }
  virtual bool SetCmd(void* pHeader, const String& strCmd) {
    if (pHeader == nullptr) return false;
    NetHeader* pH = (NetHeader*)pHeader;
    safe_printf(pH->mCmd, sizeof(pH->mCmd), "%s", strCmd.c_str());
    return true;
  }
};
extern NetDecoder g_NetHeaderDecoder;
}
