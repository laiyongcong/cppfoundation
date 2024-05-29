#include "AESCrypto.h"

namespace cppfd {
#define KEYBITS128 16
#define KEYBITS192 24
#define KEYBITS256 32

#define AES_BLOCK 16
#define AES_MATRIX_ROW 4

static uint8_t AesSbox[AES_BLOCK * AES_BLOCK] = {
    /* 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
    /*0*/ 0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    /*1*/ 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    /*2*/ 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    /*3*/ 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    /*4*/ 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    /*5*/ 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    /*6*/ 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    /*7*/ 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    /*8*/ 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    /*9*/ 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    /*a*/ 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    /*b*/ 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    /*c*/ 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    /*d*/ 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    /*e*/ 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    /*f*/ 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

static uint8_t AesiSbox[AES_BLOCK * AES_BLOCK] = {
    /* 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
    /*0*/ 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    /*1*/ 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    /*2*/ 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    /*3*/ 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    /*4*/ 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    /*5*/ 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    /*6*/ 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    /*7*/ 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    /*8*/ 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    /*9*/ 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    /*a*/ 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    /*b*/ 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    /*c*/ 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    /*d*/ 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    /*e*/ 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    /*f*/ 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d};

static uint8_t AesRcon[11 * 4] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00,
                               0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00};

class CryptoAES {
 public:
  CryptoAES(const char* pKeyData, int32_t nKeySize) {
    SetNbNkNr(nKeySize);
    nKeySize = (int32_t)strlen(pKeyData);
    if (nKeySize > (int32_t)sizeof(mKey)) nKeySize = (int32_t)sizeof(mKey);
    ::memset(mKey, 0, sizeof(mKey));
    ::memcpy(mKey, pKeyData, nKeySize);
    KeyExpansion();
  }
  ~CryptoAES(void) {}

 public:
  void Cipher(uint8_t* pInput, uint8_t* pOutput) {
    ::memset(&mState, 0, AES_BLOCK);

    for (int32_t i = 0; i < AES_MATRIX_ROW * mNb; i++) {
      mState[i % AES_MATRIX_ROW][i / AES_MATRIX_ROW] = pInput[i];
    }

    AddRoundKey(0);

    for (int32_t round = 1; round <= (mNr - 1); round++) {
      SubBytes();
      ShiftRows();
      MixColumns();
      AddRoundKey(round);
    }

    SubBytes();
    ShiftRows();
    AddRoundKey(mNr);

    for (int32_t i = 0; i < (AES_MATRIX_ROW * mNb); i++) {
      pOutput[i] = mState[i % AES_MATRIX_ROW][i / AES_MATRIX_ROW];
    }
  }

  void InvCipher(uint8_t* pInput, uint8_t* pOutput) {
    ::memset(&mState, 0, AES_BLOCK);

    for (int32_t i = 0; i < (AES_MATRIX_ROW * mNb); i++) {
      mState[i % AES_MATRIX_ROW][i / AES_MATRIX_ROW] = pInput[i];
    }

    AddRoundKey(mNr);

    for (int32_t round = mNr - 1; round >= 1; round--) {
      InvShiftRows();
      InvSubBytes();
      AddRoundKey(round);
      InvMixColumns();
    }

    InvShiftRows();
    InvSubBytes();
    AddRoundKey(0);

    for (int32_t i = 0; i < (AES_MATRIX_ROW * mNb); i++) {
      pOutput[i] = mState[i % AES_MATRIX_ROW][i / AES_MATRIX_ROW];
    }
  }

 private:
  void SetNbNkNr(int32_t nKeySize) {
    mNb = 4;
    if (nKeySize == KEYBITS128) {
      mNk = 4;
      mNr = 10;
    } else if (nKeySize == KEYBITS192) {
      mNk = 6;
      mNr = 12;
    } else if (nKeySize == KEYBITS256) {
      mNk = 8;
      mNr = 14;
    }
  }

  void AddRoundKey(int32_t nRound) {
    int32_t i = 0, j = 0;
    for (j = 0; j < AES_MATRIX_ROW; j++) {
      for (i = 0; i < AES_MATRIX_ROW; i++) {
        mState[i][j] = (uint8_t)((int32_t)mState[i][j] ^ (int32_t)mWord[AES_MATRIX_ROW * ((nRound * AES_MATRIX_ROW) + j) + i]);
      }
    }
  }

  void SubBytes(void) {
    int32_t i = 0, j = 0;
    for (j = 0; j < AES_MATRIX_ROW; j++) {
      for (i = 0; i < AES_MATRIX_ROW; i++) {
        mState[i][j] = AesSbox[mState[i][j]];
      }
    }
  }

  void InvSubBytes(void) {
    int32_t i = 0, j = 0;
    for (j = 0; j < AES_MATRIX_ROW; j++) {
      for (i = 0; i < AES_MATRIX_ROW; i++) {
        mState[i][j] = AesiSbox[mState[i][j]];
      }
    }
  }

  void ShiftRows(void) {
    uint8_t temp[AES_MATRIX_ROW * AES_MATRIX_ROW] = {0};
    int32_t i = 0, j = 0;

    for (j = 0; j < AES_MATRIX_ROW; j++) {
      for (i = 0; i < AES_MATRIX_ROW; i++) {
        temp[AES_MATRIX_ROW * i + j] = mState[i][j];
      }
    }

    for (i = 1; i < AES_MATRIX_ROW; i++) {
      for (j = 0; j < AES_MATRIX_ROW; j++) {
        if (i == 1)
          mState[i][j] = temp[AES_MATRIX_ROW * i + (j + 1) % AES_MATRIX_ROW];
        else if (i == 2)
          mState[i][j] = temp[AES_MATRIX_ROW * i + (j + 2) % AES_MATRIX_ROW];
        else if (i == 3)
          mState[i][j] = temp[AES_MATRIX_ROW * i + (j + 3) % AES_MATRIX_ROW];
      }
    }
  }

  void InvShiftRows(void) {
    uint8_t temp[AES_MATRIX_ROW * AES_MATRIX_ROW] = {0};
    int32_t i = 0, j = 0;

    for (j = 0; j < AES_MATRIX_ROW; j++) {
      for (i = 0; i < AES_MATRIX_ROW; i++) {
        temp[AES_MATRIX_ROW * i + j] = mState[i][j];
      }
    }

    for (i = 1; i < AES_MATRIX_ROW; i++) {
      for (j = 0; j < AES_MATRIX_ROW; j++) {
        if (i == 1)
          mState[i][j] = temp[AES_MATRIX_ROW * i + (j + 3) % AES_MATRIX_ROW];
        else if (i == 2)
          mState[i][j] = temp[AES_MATRIX_ROW * i + (j + 2) % AES_MATRIX_ROW];
        else if (i == 3)
          mState[i][j] = temp[AES_MATRIX_ROW * i + (j + 1) % AES_MATRIX_ROW];
      }
    }
  }

  void MixColumns(void) {
    uint8_t temp[AES_MATRIX_ROW * AES_MATRIX_ROW] = {0};
    int32_t i = 0, j = 0;

    for (j = 0; j < AES_MATRIX_ROW; j++) {
      for (i = 0; i < AES_MATRIX_ROW; i++) {
        temp[AES_MATRIX_ROW * i + j] = mState[i][j];
      }
    }

    for (j = 0; j < AES_MATRIX_ROW; j++) {
      mState[0][j] = (uint8_t)((int32_t)gfmultby02(temp[0 + j]) ^ (int32_t)gfmultby03(temp[AES_MATRIX_ROW * 1 + j]) ^ (int32_t)gfmultby01(temp[AES_MATRIX_ROW * 2 + j]) ^
                               (int32_t)gfmultby01(temp[AES_MATRIX_ROW * 3 + j]));
      mState[1][j] = (uint8_t)((int32_t)gfmultby01(temp[0 + j]) ^ (int32_t)gfmultby02(temp[AES_MATRIX_ROW * 1 + j]) ^ (int32_t)gfmultby03(temp[AES_MATRIX_ROW * 2 + j]) ^
                               (int32_t)gfmultby01(temp[AES_MATRIX_ROW * 3 + j]));
      mState[2][j] = (uint8_t)((int32_t)gfmultby01(temp[0 + j]) ^ (int32_t)gfmultby01(temp[AES_MATRIX_ROW * 1 + j]) ^ (int32_t)gfmultby02(temp[AES_MATRIX_ROW * 2 + j]) ^
                               (int32_t)gfmultby03(temp[AES_MATRIX_ROW * 3 + j]));
      mState[3][j] = (uint8_t)((int32_t)gfmultby03(temp[0 + j]) ^ (int32_t)gfmultby01(temp[AES_MATRIX_ROW * 1 + j]) ^ (int32_t)gfmultby01(temp[AES_MATRIX_ROW * 2 + j]) ^
                               (int32_t)gfmultby02(temp[AES_MATRIX_ROW * 3 + j]));
    }
  }

  void InvMixColumns(void) {
    uint8_t temp[AES_MATRIX_ROW * AES_MATRIX_ROW] = {0};
    int32_t i = 0, j = 0;

    for (i = 0; i < AES_MATRIX_ROW; i++) {
      for (j = 0; j < AES_MATRIX_ROW; j++) {
        temp[AES_MATRIX_ROW * i + j] = mState[i][j];
      }
    }

    for (j = 0; j < AES_MATRIX_ROW; j++) {
      mState[0][j] = (uint8_t)((int32_t)gfmultby0e(temp[j]) ^ (int32_t)gfmultby0b(temp[AES_MATRIX_ROW + j]) ^ (int32_t)gfmultby0d(temp[AES_MATRIX_ROW * 2 + j]) ^
                               (int32_t)gfmultby09(temp[AES_MATRIX_ROW * 3 + j]));
      mState[1][j] = (uint8_t)((int32_t)gfmultby09(temp[j]) ^ (int32_t)gfmultby0e(temp[AES_MATRIX_ROW + j]) ^ (int32_t)gfmultby0b(temp[AES_MATRIX_ROW * 2 + j]) ^
                               (int32_t)gfmultby0d(temp[AES_MATRIX_ROW * 3 + j]));
      mState[2][j] = (uint8_t)((int32_t)gfmultby0d(temp[j]) ^ (int32_t)gfmultby09(temp[AES_MATRIX_ROW + j]) ^ (int32_t)gfmultby0e(temp[AES_MATRIX_ROW * 2 + j]) ^
                               (int32_t)gfmultby0b(temp[AES_MATRIX_ROW * 3 + j]));
      mState[3][j] = (uint8_t)((int32_t)gfmultby0b(temp[j]) ^ (int32_t)gfmultby0d(temp[AES_MATRIX_ROW + j]) ^ (int32_t)gfmultby09(temp[AES_MATRIX_ROW * 2 + j]) ^
                               (int32_t)gfmultby0e(temp[AES_MATRIX_ROW * 3 + j]));
    }
  }

  FORCEINLINE uint8_t gfmultby01(uint8_t b) { return b;}

  FORCEINLINE uint8_t gfmultby02(uint8_t b) {
    if (b < 0x80)
      return (uint8_t)(int32_t)(b << 1);
    else
      return (uint8_t)((int32_t)(b << 1) ^ (int32_t)(0x1b));
  }

  uint8_t gfmultby03(uint8_t b) { return (uint8_t)((int32_t)gfmultby02(b) ^ (int32_t)b); }

  uint8_t gfmultby09(uint8_t b) { return (uint8_t)((int32_t)gfmultby02(gfmultby02(gfmultby02(b))) ^ (int32_t)b); }

  uint8_t gfmultby0b(uint8_t b) { return (uint8_t)((int32_t)gfmultby02(gfmultby02(gfmultby02(b))) ^ (int32_t)gfmultby02(b) ^ (int32_t)b); }

  uint8_t gfmultby0d(uint8_t b) { return (uint8_t)((int32_t)gfmultby02(gfmultby02(gfmultby02(b))) ^ (int32_t)gfmultby02(gfmultby02(b)) ^ (int32_t)(b)); }

  uint8_t gfmultby0e(uint8_t b) { return (uint8_t)((int32_t)gfmultby02(gfmultby02(gfmultby02(b))) ^ (int32_t)gfmultby02(gfmultby02(b)) ^ (int32_t)gfmultby02(b)); }

  void KeyExpansion(void) {
    ::memset(mWord, 0, AES_BLOCK * 15);

    for (int32_t row = 0; row < mNk; row++) {
      mWord[AES_MATRIX_ROW * row + 0] = mKey[AES_MATRIX_ROW * row];
      mWord[AES_MATRIX_ROW * row + 1] = mKey[AES_MATRIX_ROW * row + 1];
      mWord[AES_MATRIX_ROW * row + 2] = mKey[AES_MATRIX_ROW * row + 2];
      mWord[AES_MATRIX_ROW * row + 3] = mKey[AES_MATRIX_ROW * row + 3];
    }

    uint8_t temp[AES_MATRIX_ROW] = {0};
    uint8_t temp2[AES_MATRIX_ROW] = {0};

    for (int32_t row = mNk; row < AES_MATRIX_ROW * (mNr + 1); row++) {
      temp[0] = mWord[AES_MATRIX_ROW * row - AES_MATRIX_ROW];
      temp[1] = mWord[AES_MATRIX_ROW * row - 3];
      temp[2] = mWord[AES_MATRIX_ROW * row - 2];
      temp[3] = mWord[AES_MATRIX_ROW * row - 1];

      if (row % mNk == 0) {
        RotWord(temp, temp2);
        SubWord(temp2, temp);
        temp[0] = (uint8_t)((int32_t)temp[0] ^ (int32_t)AesRcon[AES_MATRIX_ROW * (row / mNk) + 0]);
        temp[1] = (uint8_t)((int32_t)temp[1] ^ (int32_t)AesRcon[AES_MATRIX_ROW * (row / mNk) + 1]);
        temp[2] = (uint8_t)((int32_t)temp[2] ^ (int32_t)AesRcon[AES_MATRIX_ROW * (row / mNk) + 2]);
        temp[3] = (uint8_t)((int32_t)temp[3] ^ (int32_t)AesRcon[AES_MATRIX_ROW * (row / mNk) + 3]);
      } else if (mNk > 6 && (row % mNk == AES_MATRIX_ROW)) {
        SubWord(temp, temp2);
        ::memcpy(temp, temp2, sizeof(temp));
      }

      mWord[AES_MATRIX_ROW * row + 0] = (uint8_t)((int32_t)mWord[AES_MATRIX_ROW * (row - mNk) + 0] ^ (int32_t)temp[0]);
      mWord[AES_MATRIX_ROW * row + 1] = (uint8_t)((int32_t)mWord[AES_MATRIX_ROW * (row - mNk) + 1] ^ (int32_t)temp[1]);
      mWord[AES_MATRIX_ROW * row + 2] = (uint8_t)((int32_t)mWord[AES_MATRIX_ROW * (row - mNk) + 2] ^ (int32_t)temp[2]);
      mWord[AES_MATRIX_ROW * row + 3] = (uint8_t)((int32_t)mWord[AES_MATRIX_ROW * (row - mNk) + 3] ^ (int32_t)temp[3]);
    }
  }

  void SubWord(uint8_t* pWord, uint8_t* pRes) {
    for (int32_t j = 0; j < AES_MATRIX_ROW; j++) {
      pRes[j] = AesSbox[AES_BLOCK * (pWord[j] >> AES_MATRIX_ROW) + (pWord[j] & 0x0f)];
    }
  }

  void RotWord(uint8_t* pWord, uint8_t* pRes) {
    pRes[0] = pWord[1];
    pRes[1] = pWord[2];
    pRes[2] = pWord[3];
    pRes[3] = pWord[0];
  }

 public:
  uint8_t mState[AES_MATRIX_ROW][AES_MATRIX_ROW];

 private:
  int32_t mNb;
  int32_t mNk;
  int32_t mNr;
  uint8_t mKey[32];
  uint8_t mWord[AES_BLOCK * 15];
};

AESUtil::AESUtil() : mAES(nullptr) {}

 AESUtil::~AESUtil() { SAFE_DELETE(mAES); }

void AESUtil::Init(const char* szKey, EAESKeyType eType) {
  SAFE_DELETE(mAES);
  switch (eType) {
    case cppfd::EAESKey_128bit:
      mAES = new CryptoAES(szKey, 16);
      break;
    case cppfd::EAESKey_192bit:
      mAES = new CryptoAES(szKey, 24);
      break;
    case cppfd::EAESKey_256bit:
      mAES = new CryptoAES(szKey, 32);
      break;
    default:
      break;
  }
}

int32_t AESUtil::Encrypt(void* pInBuff, int32_t nInLen, void* pOutBuff) {
  int32_t nOutLength = 0;

  if (mAES == NULL || pOutBuff == NULL) {
    return 0;
  }

  uint8_t* pCurInBuff = (uint8_t*)pInBuff;
  uint8_t* pCurOutBuff = (uint8_t*)pOutBuff;
  int32_t nBlockNum = nInLen / AES_BLOCK;
  int32_t nLeftNum = nInLen % AES_BLOCK;

  for (int32_t i = 0; i < nBlockNum; i++) {
    mAES->Cipher(pCurInBuff, pCurOutBuff);
    pCurInBuff += AES_BLOCK;
    pCurOutBuff += AES_BLOCK;
    nOutLength += AES_BLOCK;
  }

  if (nLeftNum) {
    uint8_t inbuff[AES_BLOCK] = {0};

    ::memcpy(inbuff, pCurInBuff, nLeftNum);
    mAES->Cipher(inbuff, pCurOutBuff);
    pCurOutBuff += AES_BLOCK;
    nOutLength += AES_BLOCK;
  }

  uint8_t extrabuff[AES_BLOCK] = {0};
  ::memset(extrabuff, 0, AES_BLOCK);

  *((int32_t*)extrabuff) = AES_BLOCK + (AES_BLOCK - nLeftNum) % AES_BLOCK;
  mAES->Cipher(extrabuff, pCurOutBuff);
  nOutLength += AES_BLOCK;

  return nOutLength;
}

int32_t AESUtil::Decrypt(void* pInBuff, int32_t nInLen, void* pOutBuff) {
  int32_t nOutLength = 0;

  if (mAES == NULL || pOutBuff == NULL) {
    return 0;
  }

  uint8_t* pCurInBuff = (uint8_t*)pInBuff;
  uint8_t* pCurOutBuff = (uint8_t*)pOutBuff;
  int32_t nBlockNum = nInLen / AES_BLOCK;
  int32_t nLeftNum = nInLen % AES_BLOCK;

  if (nLeftNum) {
    return 0;
  }

  for (int32_t i = 0; i < nBlockNum; i++) {
    mAES->InvCipher(pCurInBuff, pCurOutBuff);
    pCurInBuff += AES_BLOCK;
    pCurOutBuff += AES_BLOCK;
    nOutLength += AES_BLOCK;
  }

  uint8_t* pExtraInBuff = pCurOutBuff - AES_BLOCK;
  int32_t nExtraBytes = *((int32_t*)pExtraInBuff);

  return (nOutLength - nExtraBytes);
}

}  // namespace cppfd
