#include "RefJson.h"
#include "fasterjson.h"

namespace cppfd {

static const ReflectiveClass<JsonBase> gRefJsonBase = ReflectiveClass<JsonBase>("JsonBase");

typedef bool (*JsonTravelCallBackFunc)(void* pData, const Field& f, const char* szContent, int32_t nContentLen);
struct JsonTraveler {
  void* mCurrentData;
  const Class* mClass;
  const Field* mField;
  JsonTravelCallBackFunc mFunc;
  int32_t mIdx;
  JsonTraveler() : mCurrentData(nullptr), mClass(nullptr), mField(nullptr), mFunc(nullptr), mIdx(0) {}
};

inline bool IsNullContent(const char* szContent, uint32_t uLen) {
  if (uLen == 4 && ::memcmp(szContent, "null", 4) == 0) return true;
  return false;
}

int MyTravelJsonBuff(const char** szJson, JsonTraveler traveler);
int MyTravelLeaf(char top, const char** szJson, JsonTraveler traveler);

int MyTravelArray(char top, const char** szJson, JsonTraveler traveler){
  const char* szBegin = NULL;
  int32_t nLen = 0;
  signed char tag;
  signed char quotes;

  const char* szContent = NULL;
  int32_t nContentLen;

  char firstNode;

  int nRet = 0;
  int32_t nCount = 0;
  uint32_t nElementSize = 0;

  const Field* pField = traveler.mField;
  if (pField) {
    nCount = pField->GetElementCount();
    nElementSize = (uint32_t)pField->GetSize() / nCount;
  }
  const static Class* pVectorClass = Class::GetClassByType(typeid(JsonBase));
  void* pTempData = traveler.mCurrentData;

  traveler.mIdx = 0;
  firstNode = 1;
  while (1) {
    TOKENJSON(*szJson, szBegin, nLen, tag, quotes, FASTERJSON_ERROR_END_OF_BUFFER);
    if (tag == FASTERJSON_TOKEN_TEXT || tag == FASTERJSON_TOKEN_SPECIAL) {
      if (quotes == '\'') return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
      szContent = szBegin;
      nContentLen = nLen;

      if (IsNullContent(szContent, nContentLen) == false) {
        if (pField) {
          const Class* pClass = Class::GetClassByType(pField->GetElementType());
          if (pClass && pVectorClass->IsSuperOf(*pClass) && pTempData != NULL && ((JsonBase*)pTempData)->GetType() == EJsonArray) {
            if (nCount != 1) throw JsonParseError("JsonArray field with multi elements, field name:" + String(pField->GetName()));
            JsonBase* pVector = (JsonBase*)pTempData;
            if (traveler.mFunc) {
              pVector->AddItemByContent(szContent, nContentLen);
            }
          } else {
            if (traveler.mFunc && traveler.mIdx < nCount) {
              traveler.mCurrentData = NULL;
              if (pTempData != NULL) {
                traveler.mCurrentData = (char*)pTempData + nElementSize * traveler.mIdx;
              }
              traveler.mFunc(traveler.mCurrentData, *pField, szContent, nContentLen);
            }
          }
        } else if (traveler.mClass && pVectorClass->IsSuperOf(*traveler.mClass) && pTempData != NULL && ((JsonBase*)pTempData)->GetType() == EJsonArray) {
          JsonBase* pVector = (JsonBase*)pTempData;
          if (traveler.mFunc) {
            pVector->AddItemByContent(szContent, nContentLen);
          }
        }
      }
      traveler.mIdx++;
    } else if (tag == FASTERJSON_TOKEN_LBB) {
      const Class* pOldClass = traveler.mClass;
      if (pField) {
        const Class* pClass = Class::GetClassByType(pField->GetElementType());
        if (pClass == nullptr) throw JsonParseError("field:" + String(pField->GetName()) + " unknow element class:" + Demangle(pField->GetElementType().name()));
        if (pTempData != NULL) {
          if (pVectorClass->IsSuperOf(*pClass) && ((JsonBase*)pTempData)->GetType() == EJsonArray)  // Vector
          {
            if (nCount != 1) throw JsonParseError("JsonArray field with multi elements, field name:" + String(pField->GetName()));
            JsonBase* pVecClass = (JsonBase*)pTempData;
            if (pVecClass) {
              traveler.mCurrentData = pVecClass->AddAndGetItem();
              traveler.mClass = Class::GetClassByType(pVecClass->GetItemType());
              if (traveler.mClass == nullptr) throw JsonParseError("Unknow type:" + Demangle(pVecClass->GetItemType().name()) + " in field:" + pField->GetName());
            }
          } else {
            if (traveler.mIdx < nCount) {
              traveler.mCurrentData = (char*)pTempData + nElementSize * traveler.mIdx;
            } else {
              traveler.mCurrentData = NULL;
            }
            traveler.mClass = pClass;
          }
        } else {
          traveler.mClass = pClass;
          traveler.mCurrentData = NULL;
        }
      } else {
        const Class* pClass = traveler.mClass;
        if (pTempData != nullptr && pClass != nullptr && pVectorClass->IsSuperOf(*pClass) && ((JsonBase*)pTempData)->GetType() == EJsonArray)  // Vector
        {
          JsonBase* pVecClass = (JsonBase*)pTempData;
          if (pVecClass) {
            traveler.mCurrentData = pVecClass->AddAndGetItem();
            traveler.mClass = Class::GetClassByType(pVecClass->GetItemType());
            if (traveler.mClass == nullptr) throw JsonParseError("Unknow type:" + Demangle(pVecClass->GetItemType().name()));
          }
        }
      }

      nRet = MyTravelLeaf('{', szJson, traveler);
      traveler.mIdx++;
      traveler.mClass = pOldClass;

      if (nRet) return nRet;
    } else if (tag == FASTERJSON_TOKEN_LSB) {
      const Class* pOldClass = traveler.mClass;
      traveler.mClass = NULL;
      if (pField) {
        const Class* pTempClass = Class::GetClassByType(pField->GetElementType());
        if (pTempClass && pTempData) {
          traveler.mClass = pTempClass;
          traveler.mCurrentData = (char*)pTempData + pField->GetOffset();
        }
      }

      nRet = MyTravelArray('[', szJson, traveler);
      traveler.mClass = pOldClass;
      traveler.mCurrentData = pTempData;

      if (nRet) return nRet;
    } else if (tag == FASTERJSON_TOKEN_RSB) {
      if (firstNode == 1)
        break;
      else
        return 0;
    } else if (tag == FASTERJSON_TOKEN_RBB) {
    } else if (tag == FASTERJSON_TOKEN_COMMA) {
    } else {
      return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_ARRAY_1;
    }

    firstNode = 0;
  }

  return 0;
}

int MyTravelLeaf(char top, const char** szJson, JsonTraveler traveler) {
  char szTemp[256] = {0};
  const char* szBegin = NULL;
  int nLen = 0;
  signed char tag;
  signed char quotes, quotesBak;

  const char* szNodeName = NULL;
  int nNodeNameLen;
  const char* szContent = NULL;
  int nContentLen;

  char firstNode;

  int nRet = 0;

  void* pData = traveler.mCurrentData;

  const static Class* pMapClass = Class::GetClassByType(typeid(JsonBase));

  firstNode = 1;
  while (1) {
    TOKENJSON(*szJson, szBegin, nLen, tag, quotes, FASTERJSON_ERROR_END_OF_BUFFER);
    if (tag == FASTERJSON_TOKEN_RBB) {
      if (firstNode == 1)
        break;
      else
        return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
    } else if (tag != FASTERJSON_TOKEN_TEXT)
      return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
    quotesBak = quotes;

    szNodeName = szBegin;
    nNodeNameLen = nLen;
    safe_printf(szTemp, sizeof(szTemp), "%.*s", nNodeNameLen, szNodeName);
    if (traveler.mClass)
      traveler.mField = traveler.mClass->GetField(szTemp);
    else
      traveler.mField = NULL;

    TOKENJSON(*szJson, szBegin, nLen, tag, quotes, FASTERJSON_ERROR_END_OF_BUFFER);
    if (tag == FASTERJSON_TOKEN_COLON) {
      if (quotesBak != '\"') return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;
    } else if (tag == FASTERJSON_TOKEN_COMMA) {
      if (top != '[') return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1;

      // content = begin;
      // content_len = len;
      // 没有key， 不可能与结构体映射

      continue;
    } else if (tag == FASTERJSON_TOKEN_RBB) {
      // content = begin;
      // content_len = len;
      // 没有key， 不可能与结构体映射

      break;
    } else if (tag == FASTERJSON_TOKEN_RSB) {
      // content = begin;
      // content_len = len;
      // 没有key， 不可能与结构体映射

      break;
    } else {
      return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_2;
    }

    TOKENJSON(*szJson, szBegin, nLen, tag, quotes, FASTERJSON_ERROR_END_OF_BUFFER);
    if (tag == FASTERJSON_TOKEN_TEXT || tag == FASTERJSON_TOKEN_SPECIAL) {
      if (quotes == '\'') return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_3;

      szContent = szBegin;
      nContentLen = nLen;

      if (traveler.mFunc && traveler.mField && pData && IsNullContent(szContent, nContentLen) == false) {
        traveler.mCurrentData = (char*)pData + traveler.mField->GetOffset();
        traveler.mFunc(traveler.mCurrentData, *traveler.mField, szContent, nContentLen);
      }

      if (traveler.mClass && pMapClass->IsSuperOf(*traveler.mClass) && traveler.mCurrentData != NULL && ((JsonBase*)traveler.mCurrentData)->GetType() == EJsonMap) {
        ((JsonBase*)traveler.mCurrentData)->AddItemByContent(szContent, nContentLen, szTemp);
      }
    } else if (tag == FASTERJSON_TOKEN_LBB) {
      const Class* pOldClass = traveler.mClass;
      if (pOldClass && pMapClass->IsSuperOf(*pOldClass) && pData != nullptr && ((JsonBase*)pData)->GetType() == EJsonMap) {
        traveler.mCurrentData = ((JsonBase*)pData)->AddAndGetItem(szTemp);
        traveler.mClass = Class::GetClassByType(((JsonBase*)pData)->GetItemType());
        if (traveler.mClass == nullptr) throw JsonParseError("Unknow type:" + Demangle(((JsonBase*)pData)->GetItemType().name()));
      } else {
        traveler.mClass = NULL;
        if (pData && traveler.mField) {
          traveler.mClass = Class::GetClassByType(traveler.mField->GetType());
          if (traveler.mClass == nullptr) throw JsonParseError("Unknow type:" + Demangle(traveler.mField->GetType().name()) + " field:" + traveler.mField->GetName());
          traveler.mCurrentData = (char*)pData + traveler.mField->GetOffset();
        }
      }
      nRet = MyTravelLeaf('{', szJson, traveler);
      traveler.mClass = pOldClass;

      if (nRet) return nRet;

    } else if (tag == FASTERJSON_TOKEN_LSB) {
      szNodeName = szBegin;
      nNodeNameLen = nLen;

      const Class* pOldClass = traveler.mClass;
      traveler.mClass = NULL;
      if (pData && traveler.mField) {
        const Class* pTempClass = Class::GetClassByType(traveler.mField->GetElementType());
        if (pTempClass) {
          traveler.mClass = pTempClass;
        }
        traveler.mCurrentData = (char*)pData + traveler.mField->GetOffset();
      }

      nRet = MyTravelArray('[', szJson, traveler);
      traveler.mClass = pOldClass;
      traveler.mCurrentData = pData;

      if (nRet) return nRet;
    } else {
      return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_3;
    }

    TOKENJSON(*szJson, szBegin, nLen, tag, quotes, FASTERJSON_ERROR_END_OF_BUFFER);
    if (tag == FASTERJSON_TOKEN_COMMA) {
    } else if (tag == FASTERJSON_TOKEN_RBB) {
      break;
    } else {
      return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_4;
    }

    firstNode = 0;
  }

  return 0;
}

int MyTravelJsonBuff(const char** szJson, JsonTraveler traveler) {
  const char* szBegin = NULL;
  uint32_t nLen = 0;
  signed char tag;
  signed char quotes;

  int nRet = 0;

  while (1) {
    TOKENJSON(*szJson, szBegin, nLen, tag, quotes, FASTERJSON_INFO_END_OF_BUFFER);
    if (tag == FASTERJSON_TOKEN_LBB) {
      nRet = MyTravelLeaf('{', szJson, traveler);
      if (nRet != 0) {
        return nRet;
      }
    } else if (tag == FASTERJSON_TOKEN_LSB) {
      nRet = MyTravelArray('[', szJson, traveler);
      if (nRet != 0) {
        return nRet;
      }
    } else {
      return FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_0;
    }
  }

  return 0;
}

bool JsonBase::Content2Field(void* pDataAddr, const std::type_info& tinfo, uint32_t nCount, const char* szContent, uint32_t nContentLen) {
  if ((tinfo == typeid(char) || tinfo == typeid(signed char))) {
    if (nCount == 1) {
      int nVal = atoi(szContent);
      *(char*)pDataAddr = (char)nVal;
    } else {
      String strTmp;
      Utils::StringUnExcape(String(szContent, nContentLen), strTmp);
      char* pPos = (char*)pDataAddr;
      safe_printf(pPos, nCount, "%s", strTmp.c_str());
    }
  } else if (tinfo == typeid(std::string)) {
    if (nCount != 1) throw JsonParseError("std::string type with multi elements");
    if (nContentLen == 0)
      *(std::string*)pDataAddr = "";
    else {
      std::string& strRes = *(std::string*)pDataAddr;
      Utils::StringUnExcape(String(szContent, nContentLen), strRes);
    }
  }
  else if (tinfo == typeid(int16_t)) {
    int16_t nVal = atoi(szContent);
    *(int16_t*)pDataAddr = (int16_t)nVal;
  }
  else if (tinfo == typeid(int32_t)) {
    int32_t nVal = atoi(szContent);
    *(int32_t*)pDataAddr = nVal;
  }
  else if (tinfo == typeid(int64_t)) {
    int64_t nVal = (int64_t)atoll(szContent);
    *(int64_t*)pDataAddr = nVal;
  }
  else if (tinfo == typeid(uint8_t)) {
    uint8_t uVal = (uint8_t)atoi(szContent);
    *(uint8_t*)pDataAddr = (uint8_t)uVal;
  }
  else if (tinfo == typeid(uint16_t)) {
    uint16_t uVal = (uint16_t)atoi(szContent);
    *(uint16_t*)pDataAddr = (uint16_t)uVal;
  }
  else if (tinfo == typeid(uint32_t)) {
    uint32_t uVal = (uint32_t)atoll(szContent);
    *(uint32_t*)pDataAddr = uVal;
  }
  else if (tinfo == typeid(uint64_t)) {
    uint64_t uVal = (uint64_t)atoll(szContent);
    *(uint64_t*)pDataAddr = uVal;
  }
  else if (tinfo == typeid(float)) {
    float fVal = (float)atof(szContent);
    *(float*)pDataAddr = fVal;
  }
  else if (tinfo == typeid(double)) {
    double dVal = (double)atof(szContent);
    *(double*)pDataAddr = dVal;
  }
  else if (tinfo == typeid(bool)) {
    if (nContentLen == 4 && (::memcmp(szContent, "true", nContentLen) == 0 || ::memcmp(szContent, "True", nContentLen) == 0 || ::memcmp(szContent, "TRUE", nContentLen) == 0)) {
      *(bool*)pDataAddr = true;
    } 
    else if (nContentLen == 5 && (::memcmp(szContent, "false", nContentLen) == 0 || ::memcmp(szContent, "False", nContentLen) == 0 || ::memcmp(szContent, "FALSE", nContentLen) == 0)) {
      *(bool*)pDataAddr = false;
    }
    else if (nContentLen == 1) {
      bool bVal = atoi(szContent) > 0;
      *(bool*)pDataAddr = bVal;
    } 
    else if (nContentLen == 3 && ::memcmp(szContent, "0.0", nContentLen) == 0) {
      *(bool*)pDataAddr = false;
    } 
    else if (nContentLen == 3 && ::memcmp(szContent, "1.0", nContentLen) == 0) {
      *(bool*)pDataAddr = true;
    } else {
      throw JsonParseError("Bool Type parse error, content:" + String(szContent, nContentLen));
    }
  } else {
      throw JsonParseError("Unknow Type " + Demangle(tinfo.name()));
  }
  return true;
}

String JsonBase::Field2Json(const void* pData, const std::type_info& tInfo) {
  const static Class* pJsonClass = Class::GetClassByType(typeid(JsonBase));

  char tmpBuff[32] = {0};
  if (tInfo == typeid(int8_t)) {
    int32_t nVal = *((int8_t*)pData);
      safe_printf(tmpBuff, sizeof(tmpBuff), "%d", nVal);
  } else if (tInfo == typeid(String)) {
      String strRes;
      Utils::StringExcape(*((String*)pData), strRes);
      return "\"" + strRes + "\"";
  } else if (tInfo == typeid(int16_t)) {
      int32_t nVal = *((int16_t*)pData);
      safe_printf(tmpBuff, sizeof(tmpBuff), "%d", nVal);
  } else if (tInfo == typeid(int32_t)) {
      int32_t nVal = *((int32_t*)pData);
      safe_printf(tmpBuff, sizeof(tmpBuff), "%d", nVal);
  } else if (tInfo == typeid(int64_t)) {
      int64_t nVal = *((int64_t*)pData);
      safe_printf(tmpBuff, sizeof(tmpBuff), "%" PRId64, nVal);
  } else if (tInfo == typeid(uint8_t)) {
    uint32_t uVal = *((uint8_t*)pData);
    safe_printf(tmpBuff, sizeof(tmpBuff), "%u", uVal);
  } else if (tInfo == typeid(uint16_t)) {
    uint32_t uVal = *((uint16_t*)pData);
    safe_printf(tmpBuff, sizeof(tmpBuff), "%u", uVal);
  } else if (tInfo == typeid(uint32_t)) {
    uint32_t uVal = *((uint32_t*)pData);
    safe_printf(tmpBuff, sizeof(tmpBuff), "%u", uVal);
  } else if (tInfo == typeid(uint64_t)) {
    uint64_t uVal = *((uint64_t*)pData);
    safe_printf(tmpBuff, sizeof(tmpBuff), "%" PRIu64, uVal);
  } else if (tInfo == typeid(float)) {
    float fVal = *((float*)pData);
    safe_printf(tmpBuff, sizeof(tmpBuff), "%f", fVal);
  } else if (tInfo == typeid(double)) {
    double dVal = *((double*)pData);
    return Utils::Double3String(dVal, 15);
  } else if (tInfo == typeid(bool)) {
    bool bVal = *((bool*)pData);
    if (bVal) {
      safe_printf(tmpBuff, sizeof(tmpBuff), "true");
    } else {
      safe_printf(tmpBuff, sizeof(tmpBuff), "false");
    }
  } else {
    const Class* pClass = Class::GetClassByType(tInfo);
    if (pClass == nullptr) throw JsonWriteError("Unknow Type:" + Demangle(tInfo.name()));
    if (pJsonClass->IsSuperOf(*pClass)) {
      JsonBase* pJson = (JsonBase*)pData;
      return pJson->ToJsonString();
    } else {
      return ToJsonString(pData, *pClass);
    }
  }
  return String(tmpBuff);
}

String JsonBase::ToJsonString(const void* pAddr, const Class& refClass) { 
  const static Class* pJsonClass = Class::GetClassByType(typeid(JsonBase));
  if (pJsonClass->IsSuperOf(refClass)) return ((JsonBase*)pAddr)->ToJsonString();

  String strRes = "{";
  int nCounter = 0;
  const Class* pClass = &refClass;
  while (pClass)
  {
    const FieldList& fields = pClass->GetFields();
    for (size_t i = 0, isize = fields.size(); i < isize; i++) {
      const Field* pField = fields[i];
      const std::type_info& elementType = pField->GetElementType();
      int nElementCount = pField->GetElementCount();
      const Class* pFieldClass = Class::GetClassByType(elementType);

      JsonBase* pJson = nullptr;
      if (pFieldClass && pJsonClass->IsSuperOf(*pFieldClass) && ((JsonBase*)((char*)pAddr + pField->GetOffset()))->GetType() != EJsonObj) {
        if (nElementCount != 1) throw JsonWriteError("multi elements, json field:" + String(pField->GetName()));
        pJson = (JsonBase*)((char*)pAddr + pField->GetOffset());
        if (pJson->IsEmpty()) continue;
      }

      nCounter++;
      if (nCounter > 1) strRes.append(",");
      strRes.append("\"");
      strRes.append(pField->GetName());
      strRes.append("\":");

      if (nElementCount == 1) {
        if (pJson)
          strRes.append(pJson->ToJsonString());
        else
          strRes.append(Field2Json((char*)pAddr + pField->GetOffset(), elementType));
      } else {
        if (elementType == typeid(char)) {
          String strTmp, strExcapeTmp;
          int idx = 0;
          char* pPos = (char*)pAddr + pField->GetOffset();
          while (idx < nElementCount && *pPos) {
            strTmp.append(1, *pPos);
            idx++;
          }
          Utils::StringExcape(strTmp, strExcapeTmp);
          strRes.append("\"" + strTmp + "\"");
        } else {
          strRes.append("[");
          std::size_t uElementSize = pField->GetSize() / (std::size_t)nElementCount;
          for (int k = 0; k < nElementCount; k++) {
            strRes.append(Field2Json((char*)pAddr + pField->GetOffset() + uElementSize * k, elementType));
            if (k != nElementCount - 1) strRes.append(",");
          }
          strRes.append("]");
        }
      }
    }
    pClass = pClass->Super();
  }
  strRes.append("}");
  return strRes;
}

bool JsonTravelCallBack(void* pData, const Field& f, const char* szContent, int32_t nContentLen) {
  if (pData == NULL) {
    return true;
  }

  return JsonBase::Content2Field(pData, f.GetElementType(), f.GetElementCount(), szContent, nContentLen);
}

bool JsonBase::FromJsonString(void* pAddr, const Class& refClass, const String& strJson) { 
  JsonTraveler traveler;
  traveler.mClass = &refClass;
  traveler.mCurrentData = pAddr;
  traveler.mFunc = JsonTravelCallBack;
  const char* szBuff = strJson.c_str();

  int nRet = MyTravelJsonBuff(&szBuff, traveler);

  return (nRet == 0 || nRet == FASTERJSON_INFO_END_OF_BUFFER) ? true : false;
}

bool JsonBase::IsBuildInType(const std::type_info& tinfo) {
  if (tinfo == typeid(int8_t) || 
      tinfo == typeid(uint8_t) || 
      tinfo == typeid(int16_t) || 
      tinfo == typeid(uint16_t) || 
      tinfo == typeid(int32_t) || 
      tinfo == typeid(uint32_t) ||
      tinfo == typeid(int64_t) || 
      tinfo == typeid(uint64_t) || 
      tinfo == typeid(float) || 
      tinfo == typeid(double) || 
      tinfo == typeid(bool) || 
      tinfo == typeid(String)
  )
      return true;
  return false;
}

}