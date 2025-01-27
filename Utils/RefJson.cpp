#include "RefJson.h"
#include "fasterjson.h"

namespace cppfd {

static const ReflectiveClass<JsonBase> gRefJsonBase = ReflectiveClass<JsonBase>("JsonBase");
static const ReflectiveClass<JsonArray<int8_t> > gRefInt8Arr = ReflectiveClass<JsonArray<int8_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<char> > gRefCharArr = ReflectiveClass<JsonArray<char> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<uint8_t> > gRefUInt8Arr = ReflectiveClass<JsonArray<uint8_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<int16_t> > gRefInt16Arr = ReflectiveClass<JsonArray<int16_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<uint16_t> > gRefUInt16Arr = ReflectiveClass<JsonArray<uint16_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<int32_t> > gRefInt32Arr = ReflectiveClass<JsonArray<int32_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<uint32_t> > gRefUInt32Arr = ReflectiveClass<JsonArray<uint32_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<int64_t> > gRefInt64Arr = ReflectiveClass<JsonArray<int64_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<uint64_t> > gRefUInt64Arr = ReflectiveClass<JsonArray<uint64_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<bool> > gRefBoolArr = ReflectiveClass<JsonArray<bool> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<float> > gRefFloatArr = ReflectiveClass<JsonArray<float> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<double> > gRefDoubleArr = ReflectiveClass<JsonArray<double> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonArray<String> > gRefStringArr = ReflectiveClass<JsonArray<String> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<int8_t> > gRefInt8Map = ReflectiveClass<JsonMap<int8_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<char> > gRefCharMap = ReflectiveClass<JsonMap<char> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<uint8_t> > gRefUInt8Map = ReflectiveClass<JsonMap<uint8_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<int16_t> > gRefInt16Map = ReflectiveClass<JsonMap<int16_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<uint16_t> > gRefUInt16Map = ReflectiveClass<JsonMap<uint16_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<int32_t> > gRefInt32Map = ReflectiveClass<JsonMap<int32_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<uint32_t> > gRefUInt32Map = ReflectiveClass<JsonMap<uint32_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<int64_t> > gRefInt64Map = ReflectiveClass<JsonMap<int64_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<uint64_t> > gRefUInt64Map = ReflectiveClass<JsonMap<uint64_t> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<bool> > gRefBoolMap = ReflectiveClass<JsonMap<bool> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<float> > gRefFloatMap = ReflectiveClass<JsonMap<float> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<double> > gRefDoubleMap = ReflectiveClass<JsonMap<double> >((JsonBase*)nullptr);
static const ReflectiveClass<JsonMap<String> > gRefStringMap = ReflectiveClass<JsonMap<String> >((JsonBase*)nullptr);

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

int MyTravelJsonBuff(const char** szJson, JsonTraveler& traveler);
int MyTravelLeaf(char top, const char** szJson, JsonTraveler& traveler);

int MyTravelArray(char top, const char** szJson, JsonTraveler& traveler){
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
  const static Class* pVectorClass = Class::GetClass(typeid(JsonBase));
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
          const Class* pClass = Class::GetClass(pField->GetElementType());
          if (pClass && pVectorClass->IsSuperOf(*pClass) && pTempData != NULL && ((JsonBase*)pTempData)->GetJsonType() == EJsonArray) {
            if (nCount != 1) LOG_ERROR("JsonArray field with multi elements, field name:%s", pField->GetName());
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
        } else if (traveler.mClass && pVectorClass->IsSuperOf(*traveler.mClass) && pTempData != NULL && ((JsonBase*)pTempData)->GetJsonType() == EJsonArray) {
          JsonBase* pVector = (JsonBase*)pTempData;
          if (traveler.mFunc) {
            pVector->AddItemByContent(szContent, nContentLen);
          }
        }
      }
      traveler.mIdx++;
    } else if (tag == FASTERJSON_TOKEN_LBB) {
      const Class* pOldClass = traveler.mClass;
      JsonBase* pJsonBase = nullptr;
      void* pNewItem = nullptr;
      if (pField) {
        const Class* pClass = Class::GetClass(pField->GetElementType());
        if (pClass == nullptr) {
          LOG_ERROR("field:%s unknow element class:%s", pField->GetName(), Demangle(pField->GetElementType().name()).c_str());
        } else {
          if (pTempData != nullptr) {
            if (pVectorClass->IsSuperOf(*pClass) && ((JsonBase*)pTempData)->GetJsonType() == EJsonArray)  // Vector
            {
              if (nCount != 1) LOG_ERROR("JsonArray field with multi elements, field name:%s", pField->GetName());
              pJsonBase = (JsonBase*)pTempData;
              if (pJsonBase) {
                traveler.mClass = Class::GetClass(pJsonBase->GetItemType());
                if (traveler.mClass == nullptr) {
                  LOG_ERROR("Unknow type:%s in field:%s", Demangle(pJsonBase->GetItemType().name()).c_str(), pField->GetName());
                  return -1;
                }
                pNewItem = pJsonBase->NewItem();
                traveler.mCurrentData = pNewItem;
              }
            } else {
              if (traveler.mIdx < nCount) {
                traveler.mCurrentData = (char*)pTempData + nElementSize * traveler.mIdx;
              } else {
                traveler.mCurrentData = nullptr;
              }
              traveler.mClass = pClass;
            }
          } else {
            traveler.mClass = pClass;
            traveler.mCurrentData = nullptr;
          }
        }
      } else {
        const Class* pClass = traveler.mClass;
        if (pTempData != nullptr && pClass != nullptr && pVectorClass->IsSuperOf(*pClass) && ((JsonBase*)pTempData)->GetJsonType() == EJsonArray)  // Vector
        {
          pJsonBase = (JsonBase*)pTempData;
          if (pJsonBase) {
            traveler.mClass = Class::GetClass(pJsonBase->GetItemType());
            if (traveler.mClass == nullptr) {
              LOG_ERROR("Unknow type:%s", Demangle(pJsonBase->GetItemType().name()).c_str());
              return -1;
            }
            pNewItem = pJsonBase->NewItem();
            traveler.mCurrentData = pNewItem;
          }
        }
      }

      nRet = MyTravelLeaf('{', szJson, traveler);
      if (pNewItem && pJsonBase) pJsonBase->AddItem(pNewItem); 
      traveler.mIdx++;
      traveler.mClass = pOldClass;

      if (nRet) return nRet;
    } else if (tag == FASTERJSON_TOKEN_LSB) {
      const Class* pOldClass = traveler.mClass;
      traveler.mClass = NULL;
      if (pField) {
        const Class* pTempClass = Class::GetClass(pField->GetElementType());
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

int MyTravelLeaf(char top, const char** szJson, JsonTraveler& traveler) {
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

  const static Class* pMapClass = Class::GetClass(typeid(JsonBase));

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

      if (traveler.mClass && pMapClass->IsSuperOf(*traveler.mClass) && traveler.mCurrentData != NULL && ((JsonBase*)traveler.mCurrentData)->GetJsonType() == EJsonMap) {
        ((JsonBase*)traveler.mCurrentData)->AddItemByContent(szContent, nContentLen, szTemp);
      }
    } else if (tag == FASTERJSON_TOKEN_LBB) {
      const Class* pOldClass = traveler.mClass;
      JsonBase* pJsonBase = nullptr;
      void* pNewItem = nullptr;

      if (pOldClass && pMapClass->IsSuperOf(*pOldClass) && pData != nullptr && ((JsonBase*)pData)->GetJsonType() == EJsonMap) {
        pJsonBase = (JsonBase*)pData;
        traveler.mClass = Class::GetClass(((JsonBase*)pData)->GetItemType());
        if (traveler.mClass == nullptr) {
          LOG_ERROR("Unknow type:%s", Demangle(pJsonBase->GetItemType().name()).c_str());
          return -1;
        }
        pNewItem = pJsonBase->NewItem();
        traveler.mCurrentData = pNewItem;
      } else {
        traveler.mClass = NULL;
        if (pData && traveler.mField) {
          traveler.mClass = Class::GetClass(traveler.mField->GetType());
          if (traveler.mClass == nullptr) {
            LOG_ERROR("Unknow type:%s in field:%s", Demangle(traveler.mField->GetType().name()).c_str(), traveler.mField->GetName());
            return -1;
          }
          traveler.mCurrentData = (char*)pData + traveler.mField->GetOffset();
        }
      }
      nRet = MyTravelLeaf('{', szJson, traveler);
      traveler.mClass = pOldClass;
      if (pNewItem && pJsonBase) pJsonBase->AddItem(pNewItem, szTemp);

      if (nRet) return nRet;

    } else if (tag == FASTERJSON_TOKEN_LSB) {
      szNodeName = szBegin;
      nNodeNameLen = nLen;

      const Class* pOldClass = traveler.mClass;
      traveler.mClass = NULL;
      if (pData && traveler.mField) {
        const Class* pTempClass = Class::GetClass(traveler.mField->GetElementType());
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

int MyTravelJsonBuff(const char** szJson, JsonTraveler& traveler) {
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
  if ((tinfo == typeid(char) || tinfo == typeid(int8_t))) {
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
    if (nCount != 1) {
      LOG_ERROR("std::string type with multi elements");
      return false;
    }
    if (nContentLen == 0)
      *(std::string*)pDataAddr = "";
    else {
      std::string& strRes = *(std::string*)pDataAddr;
      Utils::StringUnExcape(String(szContent, nContentLen), strRes);
    }
  } else if (tinfo == typeid(int16_t)) {
    int16_t nVal = atoi(szContent);
    *(int16_t*)pDataAddr = (int16_t)nVal;
  } else if (tinfo == typeid(int32_t)) {
    int32_t nVal = atoi(szContent);
    *(int32_t*)pDataAddr = nVal;
  } else if (tinfo == typeid(int64_t)) {
    int64_t nVal = (int64_t)atoll(szContent);
    *(int64_t*)pDataAddr = nVal;
  } else if (tinfo == typeid(uint8_t)) {
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
    if (nContentLen == 1 && (szContent[0] == 't' || szContent[0] == 'T')) {
      *(bool*)pDataAddr = true;
    } 
    else if (nContentLen == 1 && (szContent[0] == 'f' || szContent[0] == 'F')) {
      *(bool*)pDataAddr = false;
    }
    else if (nContentLen == 1) {
      bool bVal = atoi(szContent) > 0;
      *(bool*)pDataAddr = bVal;
    } else if (nContentLen == 4 && tstrnicmp("true", szContent, nContentLen) == 0) {
      *(bool*)pDataAddr = true;
    } else if (nContentLen == 5 && tstrnicmp("false", szContent, nContentLen) == 0) {
      *(bool*)pDataAddr = false;
    }
    else if (nContentLen == 3 && ::memcmp(szContent, "0.0", nContentLen) == 0) {
      *(bool*)pDataAddr = false;
    } 
    else if (nContentLen == 3 && ::memcmp(szContent, "1.0", nContentLen) == 0) {
      *(bool*)pDataAddr = true;
    } else {
      LOG_ERROR("Bool Type parse error, content:%.*s", nContentLen, szContent);
      return false;
    }
  } else {
    LOG_ERROR("Unknow Type %s", Demangle(tinfo.name()).c_str());
    return false;
  }
  return true;
}

String JsonBase::Field2Json(const void* pData, const std::type_info& tInfo) {
  const static Class* pJsonClass = Class::GetClass(typeid(JsonBase));

  char tmpBuff[32] = {0};
  if (tInfo == typeid(int8_t) || tInfo == typeid(char)) {
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
    return Utils::Double2String(dVal, 15);
  } else if (tInfo == typeid(bool)) {
    bool bVal = *((bool*)pData);
    if (bVal) {
      safe_printf(tmpBuff, sizeof(tmpBuff), "true");
    } else {
      safe_printf(tmpBuff, sizeof(tmpBuff), "false");
    }
  } else {
    const Class* pClass = Class::GetClass(tInfo);
    if (pClass == nullptr) {
      LOG_ERROR("Unknow Type:%s", Demangle(tInfo.name()).c_str());
      return "";
    }
    if (pJsonClass->IsSuperOf(*pClass)) {
      JsonBase* pJson = (JsonBase*)pData;
      return pJson->ToJsonString();
    } else {
      return ToJsonString(pData, *pClass);
    }
  }
  return String(tmpBuff);
}

String JsonBase::ToJsonString(const void* pAddr, const Class& refClass, std::function<bool(const Field*)> filterFunc /*= nullptr*/) { 
  const static Class* pJsonClass = Class::GetClass(typeid(JsonBase));
  if (pJsonClass->IsSuperOf(refClass)) return ((JsonBase*)pAddr)->ToJsonString();

  String strRes = "{";
  int nCounter = 0;
  const Class* pClass = &refClass;
  while (pClass)
  {
    const FieldList& fields = pClass->GetFields();
    for (size_t i = 0, isize = fields.size(); i < isize; i++) {
      const Field* pField = fields[i];
      if (filterFunc != nullptr && !filterFunc(pField)) continue; 
      const std::type_info& elementType = pField->GetElementType();
      int nElementCount = pField->GetElementCount();
      const Class* pFieldClass = Class::GetClass(elementType);

      JsonBase* pJson = nullptr;
      if (pFieldClass && pJsonClass->IsSuperOf(*pFieldClass) && ((JsonBase*)((char*)pAddr + pField->GetOffset()))->GetJsonType() != EJsonObj) {
        if (nElementCount != 1) {
          LOG_ERROR("multi elements, json field:%s", pField->GetName());
          continue;
        }
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
      tinfo == typeid(char) || 
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