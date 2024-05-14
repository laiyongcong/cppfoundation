#pragma once
#include "NetApi.h"
#include "Utils.h"
#include "Queue.h"
#include "Reflection.h"

namespace cppfd {
class Connecter;
class TcpEngine;
class NetThread;
struct NetIOInfo;

typedef uint8_t (*NetCryptoFunc)(uint64_t&);  //网络字节混淆函数指针

extern uint8_t DefaultNetCryptoFunc(uint64_t& ulSeed);

#define MSG_HANDLER_FUNC(FUNC) REF_EXPORT static int FUNC(cppfd::Connecter* pConn, const char* szBuff, uint32_t uBuffLen)

struct Pack {
  virtual const char* GetBuff() = 0;
  virtual uint32_t GetDataLen() const = 0; //数据长度，包含包头
};

class Connecter : public WeakPtrArray::Item, public NonCopyable {
  friend class TcpEngine;
  friend class NetThread;
  friend struct NetIOInfo;
 public:
  Connecter();
  ~Connecter();

  bool Send(const String& strCmd, const char* szPack, uint32_t uPackLen);
  FORCEINLINE String Info() const { return "ConnecterID:" + std::to_string(mConnecterID) + " ip:" + mPeerIp + " port:" + std::to_string(mPeerPort); }
  FORCEINLINE uint32_t GetConnecterID() const { return mConnecterID; }
  FORCEINLINE String GetPeerIp() const { return mPeerIp; }
  FORCEINLINE int GetPeerPort() const { return mPeerPort; }
  void Kick(const String& strMsg);
 private:
  SOCKET mSock;
  String mPeerIp;
  int mPeerPort;
  uint32_t mConnecterID;
  NetThread* mNetThread;
  NetIOInfo* mIO;
};

class TcpEngine : public NonCopyable {
  friend class NetThread;
  friend class ConnecterWorkerThread;
 public:
  TcpEngine(uint32_t uNetThreadNum, BaseNetDecoder* pDecoder, const std::type_info& tMsgClass, int nPort, const String& strHost = "0.0.0.0");
  virtual ~TcpEngine();
  
  void SetCrypto(NetCryptoFunc SendCryptoFunc, NetCryptoFunc RecvCryptoFunc, uint64_t uSendCrypto, uint64_t uRecvCrypto);//设置字节混淆方法，只能在start之前调用
  bool Start();

  bool Connect(const char* szHost, int nPort, int* pMicroTimeout, bool bLingerOn = true, uint32_t uLinger = 0, int nClientPort = 0);
 public:
  virtual void OnConnecterCreate(Connecter* pConn);                  // 被网络线程调用，
  virtual void OnConnecterClose(std::shared_ptr<Connecter> pConn, const String& szErrMsg); // 被网络线程调用，
  virtual int OnRecvMsg(Connecter* pConn, Pack* pPack); // 被网络线程调用
  virtual Connecter* AllocateConnecter() { return new (std::nothrow) Connecter; } //若用户继承并扩展了connecter，需要override此函数
 private:
  NetThread* AllocateNetThread();
 protected:
  uint32_t mNetThreadNum;
  String mHost;
  int mPort;
  NetThread* mNetThreads;
  const Class* mMsgClass;

  uint64_t mSendCryptoSeed;
  uint64_t mRecvCryptoSeed;
  BaseNetDecoder* mDecoder;
  NetCryptoFunc mSendCryptoFunc;
  NetCryptoFunc mRecvCryptoFunc;

 private:
  uint32_t mHeaderSize;
};

class TcpClient {
 public:
  static bool Connect(SOCKET& sock, const char* szHost, int nPort, int* pMicroTimeout, bool bLingerOn = true, uint32_t uLinger = 0, int nClientPort = 0);
  static int Read(SOCKET sock, void* szBuf, int len, int* pMicroTimeout);
  static int Write(SOCKET sock, void* szBuf, int len, int* pMicroTimeout);
};

}