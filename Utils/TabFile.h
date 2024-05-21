#pragma once
#include "Prerequisites.h"
#include "RefJson.h"
#include "Log.h"

namespace cppfd {
template <typename T, const char Delim>
class TabFile {
 public:
  std::vector<T> mItems;

 public:
  bool OpenFromTxt(const char* szTabFile, int nIgnoreLines = 0) {
    if (szTabFile == nullptr || szTabFile[0] == 0) {
      LOG_WARNING("empty file name");
      return false;
    }
    const Class* pClass = Class::GetClass(typeid(T));
    if (pClass == nullptr){
      LOG_WARNING("No Reflection info of:%s while reading file:%s", Demangle(typeid(T).name()).c_str(), szTabFile);
      return false;
    }

    FILE* fp = fopen(szTabFile, "rb");
    if (nullptr == fp) {
      LOG_ERROR("open file:%s failed!", szTabFile);
      return false;
    }

    fseek(fp, 0, SEEK_END);
    int nFileSize = ftell(fp);
    if (nFileSize < 0) {
      fclose(fp);
      LOG_ERROR("nFileSize:%d error reading file:%s", nFileSize, szTabFile);
      return false;
    }

    fseek(fp, 0, SEEK_SET);

    size_t nSize = (size_t)nFileSize;

    // 读入内存
    char* pMemory = new char[nSize + 1];
    size_t nReadCount = fread(pMemory, 1, nSize, fp);
    fclose(fp);
    pMemory[nSize] = 0;
    if (nReadCount != nSize) {
      SAFE_DELETE_ARRAY(pMemory);
      LOG_ERROR("read file:%s failed!", szTabFile);
      return false;
    }

    bool bret = OpenFromMemory(pMemory, pMemory + nSize + 1, pClass, szTabFile, nIgnoreLines);

    SAFE_DELETE_ARRAY(pMemory);

    return bret;
  }

  bool Save(const char* szTabFile) const {
    if (szTabFile == nullptr || szTabFile[0] == 0) {
      LOG_WARNING("empty file name");
      return false;
    }
    const Class* pClass = Class::GetClass(typeid(T));
    if (pClass == nullptr) {
      LOG_WARNING("No Reflection info of:%s while saving file:%s", Demangle(typeid(T).name()).c_str(), szTabFile);
      return false;
    }

    FILE* fp = fopen(szTabFile, "wb");
    if (nullptr == fp) {
      LOG_ERROR("open file:%s failed!", szTabFile);
      return false;
    }

    std::ostringstream outStream;
    const FieldList& fields = pClass->GetFields();
    for (size_t i = 0, isize = fields.size(); i < isize; i++) {
      const Field* f = fields[i];
      String strName = f->GetName();
      outStream << strName;
      if (i != isize - 1) {
        outStream << Delim;
      }
    }
    outStream << "\n";
    for (auto& it : mItems) {
      for (size_t i = 0, isize = fields.size(); i < isize; i++) {
        const Field* pField = fields[i];
        int nCount = pField->GetElementCount();
        const std::type_info& eType = pField->GetElementType();

        const Class* pElementClass = Class::GetClass(eType);
        if (pElementClass != nullptr) {
          if (nCount != 1) {
            LOG_WARNING("type:%s array is not supported!", Demangle(eType.name()).c_str());
            fclose(fp);
            return false;
          }
          String strJson = JsonBase::ToJsonString((const void*)((char*)&it + pField->GetOffset()), *pElementClass);
          outStream << "\"" << Utils::StringExcapeChar(strJson, '"') << "\"";
          if (i != isize - 1) outStream << Delim;
          continue;
        }
        
        if (nCount > 1) outStream << "\"";
        if (eType == typeid(int8_t)) {
          for (int j = 0; j < nCount; j++) {
            int8_t val = 0;
            pField->GetByIdx(&it, val, j);
            if (val == 0) break;
            outStream << val;
            if (val == '"') outStream << val;
          }
        } else if (eType == typeid(char)) {
          for (int j = 0; j < nCount; j++) {
            char val = 0;
            pField->GetByIdx(&it, val, j);
            if (val == 0) break;
            outStream << val;
            if (val == '"') outStream << val;
          }
        } else if (eType == typeid(uint8_t)) {
          uint8_t val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else if (eType == typeid(int16_t)) {
          int16_t val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else if (eType == typeid(uint16_t)) {
          uint16_t val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else if (eType == typeid(int32_t)) {
          int32_t val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else if (eType == typeid(uint32_t)) {
          uint32_t val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else if (eType == typeid(int64_t)) {
          int64_t val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else if (eType == typeid(uint64_t)) {
          uint64_t val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else if (eType == typeid(String)) {
          String val = "";
          pField->GetByIdx(&it, val, 0);
          outStream << Utils::StringExcapeChar(val, '"');
          for (int j = 1; j < nCount; j++) {
            outStream << Utils::StringExcapeChar(val, '"');
            outStream << "," << val;
          }
        } else if (eType == typeid(float)) {
          float val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else if (eType == typeid(double)) {
          double val = 0;
          pField->GetByIdx(&it, val, 0);
          outStream << Utils::Double2String(val, 15);
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << Utils::Double2String(val, 15);
          }
        } else if (eType == typeid(bool)) {
          bool val = false;
          pField->GetByIdx(&it, val, 0);
          outStream << val;
          for (int j = 1; j < nCount; j++) {
            pField->GetByIdx(&it, val, j);
            outStream << "," << val;
          }
        } else {
          LOG_WARNING("unknow field type:%s", Demangle(eType.name()).c_str());
          fclose(fp);
          return false;
        }
        if (nCount > 1) outStream << "\"";
        if (i != isize - 1) outStream << Delim;
      }
      outStream << "\n";
    }

    fwrite(outStream.str().c_str(), outStream.str().size(), 1, fp);
    fclose(fp);
    return true;
  }
 private:
  const char* GetLineFromMemory(char* pStringBuf, int nBufSize, const char* pMemory, const char* pDeadEnd) {
    const char* pMem = pMemory;
    if (pMem >= pDeadEnd || *pMem == 0) return 0;

    while (pMem < pDeadEnd && pMem - pMemory + 1 < nBufSize && *pMem != 0 && *pMem != '\r' && *pMem != '\n') *(pStringBuf++) = *(pMem++);
    // add 'null' end
    *pStringBuf = 0;

    // skip all next \r and \n
    while (pMem < pDeadEnd && *pMem != 0 && (*pMem == '\r' || *pMem == '\n')) pMem++;

    return pMem;
  }

  uint32_t ConvertStringToVector(const char* strStrINTgSource, std::vector<String>& vRet, const char szDelim, bool bIgnoreEmpty) {
    vRet.clear();
    String strCell = "";
    strCell.reserve(strlen(strStrINTgSource));
    const char* pReadAt = strStrINTgSource;
    bool bQuoted = false;
    bool bHaveTail = false;

    while (*pReadAt) {
      if (*pReadAt == '"') {
        uint32_t nQuotesNum = 0;
        while (*(pReadAt + nQuotesNum) == '"') ++nQuotesNum;
        pReadAt += nQuotesNum;

        if (nQuotesNum % 2 != 0) {
          bQuoted = !bQuoted;
        }

        nQuotesNum /= 2;
        while (nQuotesNum-- > 0) strCell.append(1, '"');
        continue;
      } else {
        if (bQuoted) {
          strCell.append(1, *pReadAt);
          ++pReadAt;
          continue;
        }
        if (*pReadAt == szDelim) {
          if (strCell == "\"") strCell = "";
          if (strCell != "" || !bIgnoreEmpty) {
            vRet.push_back(strCell);
          }
          strCell.clear();
          bHaveTail = true;
          ++pReadAt;
          continue;
        }
        strCell.append(1, *pReadAt);
        ++pReadAt;
      }
    }
    if (bHaveTail) {
      if (strCell == "\"") strCell = "";
      if (strCell != "" || !bIgnoreEmpty) {
        vRet.push_back(strCell);
      }
    }
    return (uint32_t)vRet.size();
  }

  template <typename V1, typename V2>
  bool ConvertStr2Field(T* object, const String& str, const Field& f, V2 (*transfunc)(const char*)) {
    size_t ncount = f.GetElementCount();
    std::vector<std::string> vRet;

    if (ncount == 1) {
      V1 val = (V1)transfunc(str.c_str());
      f.Set(object, val);
      return true;
    }
    ConvertStringToVector(str.c_str(), vRet, ',', false);
    size_t len = vRet.size();
    len = len > ncount ? ncount : len;
    for (size_t i = 0; i < len; i++) {
      V1 val = (V1)transfunc(vRet[i].c_str());
      f.SetByIdx(object, val, (int)i);
    }
    return true;
  }

  bool Str2ObjField(const String& str, T* pObj, const Field& field) {
    if (pObj == nullptr) return false;
    if (str == "") return true;
    int nCount = field.GetElementCount();
    std::vector<String> vRet;
    const std::type_info& eType = field.GetElementType();

    if (eType == typeid(int8_t) || eType == typeid(char)) {
      String strTmp = Utils::StringUnExcapeChar(str, '"');
      char* pAddr = (char*)pObj + field.GetOffset(); 
      int isize = (int)strTmp.size();
      for (int i = 0; i < nCount && i < isize; i++) {
        *pAddr = strTmp[i];
      }
      if (nCount > 1) {
        pAddr[nCount - 1] = 0;
        if (isize < nCount) pAddr[isize] = 0;
      }
      return true;
    } else if (eType == typeid(uint8_t)) {
      return ConvertStr2Field<uint8_t>(pObj, str, field, atoi);
    } else if (eType == typeid(int16_t)) {
      return ConvertStr2Field<int16_t>(pObj, str, field, atoi);
    } else if (eType == typeid(uint16_t)) {
      return ConvertStr2Field<uint16_t>(pObj, str, field, atoi);
    } else if (eType == typeid(int32_t)) {
      return ConvertStr2Field<int32_t>(pObj, str, field, atoi);
    } else if (eType == typeid(uint32_t)) {
      return ConvertStr2Field<uint32_t>(pObj, str, field, atoi);
    } else if (eType == typeid(int64_t)) {
      return ConvertStr2Field<int64_t>(pObj, str, field, atoll);
    } else if (eType == typeid(uint64_t)) {
      return ConvertStr2Field<uint64_t>(pObj, str, field, atoll);
    } else if (eType == typeid(String)) {
      ConvertStringToVector(str.c_str(), vRet, ',', false);
      for (int i = 0, nSize = (int)vRet.size(); i < nCount && i < nSize; i++) field.SetByIdx(pObj, Utils::StringUnExcapeChar(vRet[i], '"'), i);
      return true;
    } else if (eType == typeid(float)) {
      return ConvertStr2Field<float>(pObj, str, field, atof);
    } else if (eType == typeid(double)) {
      return ConvertStr2Field<double>(pObj, str, field, atof);
    } else if (eType == typeid(bool)) {
      String strTmp = str;
      Utils::String2LowerCase(strTmp);
      ConvertStringToVector(strTmp.c_str(), vRet, ',', false);
      for (int i = 0, nSize = (int)vRet.size(); i < nCount && i < nSize; i++) {
        bool bVal = false;
        if (vRet[i] == "true")
          bVal = true;
        else if (vRet[i] != "false") {
          bVal = atoi(vRet[i].c_str()) != 0;
        }
        field.SetByIdx(pObj, bVal, i);
      }
      return true;
    }
    static const Class* pBaseClass = Class::GetClass<JsonBase>();
    const Class* pClass = Class::GetClass(eType);
    if (pClass == nullptr) {
      LOG_WARNING("unknow field type:%s", Demangle(eType.name()).c_str());
      return false;
    }
    if (nCount != 1) {
      LOG_WARNING("type:%s array is not supported!", Demangle(eType.name()).c_str());
      return false;
    }
    String strTmp = str;
    if (pBaseClass->IsSuperOf(*pClass)) {
      JsonBase* pBase = (JsonBase*)((char*)pObj + field.GetOffset());
      auto eJsonType = pBase->GetJsonType();
      if (eJsonType == EJsonArray) {
        if (strTmp[0] != '[') strTmp = "[" + strTmp + "]";
      } else if (strTmp[0] != '{') {
        strTmp = "{" + strTmp + "}";
      }
    } else if (strTmp[0] != '{') {
      strTmp = "{" + strTmp + "}";
    }
    return JsonBase::FromJsonString((char*)pObj + field.GetOffset(), *pClass, Utils::StringUnExcapeChar(strTmp, '"'));
  }

  bool OpenFromMemory(const char* pMemory, const char* pDeadEnd, const Class* pClass, const char* szFileName, int nIgnoreLines) {
    if (pMemory == nullptr || pDeadEnd == nullptr || pClass == nullptr || szFileName == nullptr) {
      LOG_WARNING("invalid param in OpenFromMemory");
      return false;
    }
    char szLine[1024 * 10] = {0};
    std::vector<std::string> vRet;
    std::vector<std::string> vFieldName;
    // 读取首行，分析对应的字段
    const char* pMem = pMemory;
    while ((pMem = GetLineFromMemory(szLine, 1024 * 10, pMem, pDeadEnd)) != nullptr) {
      if (szLine[0] == '#') continue;
      break;
    }
    if (pMem == nullptr) return false;

    ConvertStringToVector(szLine, vFieldName, Delim, true);
    if (vFieldName.empty()) return false;

    // 处理忽略的数据
    while (nIgnoreLines > 0 && pMem) {
      pMem = GetLineFromMemory(szLine, 1024 * 10, pMem, pDeadEnd);
      if (szLine[0] != '#') {
        nIgnoreLines--;
      }
    }
    if (pMem == nullptr) return false;

    mItems.clear();
    size_t fieldCount = vFieldName.size();
    std::map<std::string, const Field*> FieldsMap;
    // 检验是否有未对应的字段，并输出警告
    for (size_t i = 0; i < fieldCount; i++) {
      std::string& str = vFieldName[i];
      const Field* f = pClass->GetField(str.c_str());
      if (f == nullptr) continue;
      FieldsMap[str] = f;
    }

    const FieldList& fieldList = pClass->GetFields();
    for (size_t i = 0; i < fieldList.size(); i++) {
      const Field* pf = fieldList[i];
      if (FieldsMap.find(pf->GetName()) == FieldsMap.end()) {
        LOG_WARNING("Unknown initialization mode of field:%s in struct:%s, tabfile:%s\n", pf->GetName(), Demangle(typeid(T).name()).c_str(), szFileName);
      }
    }

    int32_t nRowIdx = 1;
    while ((pMem = GetLineFromMemory(szLine, 1024 * 10, pMem, pDeadEnd)) != nullptr) {
      nRowIdx++;
      if (szLine[0] == '#') continue;
      // 分解
      ConvertStringToVector(szLine, vRet, Delim, false);
      if (vRet.empty()) continue;

      size_t retCount = vRet.size();
      if (retCount > fieldCount) retCount = fieldCount;

      T tItem;
      for (size_t i = 0; i < retCount; i++) {
        const Field* f = FieldsMap[vFieldName[i]];
        if (f == nullptr) continue;
        if(!Str2ObjField(vRet[i], &tItem, *f)) {
          LOG_WARNING("Read TabFile:%s Row:%d, Col:%u, name:%s failed", szFileName, nRowIdx, (uint32_t)i, f->GetName());
          continue;
        }
      }
      mItems.push_back(tItem);
    }
    return true;
  }
};

}