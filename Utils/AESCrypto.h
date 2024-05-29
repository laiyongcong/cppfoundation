#pragma once
#include "Prerequisites.h"

namespace cppfd {
enum EAESKeyType {
	EAESKey_128bit,
	EAESKey_192bit,
	EAESKey_256bit
};

class CryptoAES;
class AESUtil {
 public:
  AESUtil();
  ~AESUtil();

 public:
  void Init(const char* szKey, EAESKeyType eType);
  int32_t Encrypt(void* pInBuff, int32_t nInLen, void* pOutBuff);
  int32_t Decrypt(void* pInBuff, int32_t nInLen, void* pOutBuff);
 private:
  CryptoAES* mAES;
};
}