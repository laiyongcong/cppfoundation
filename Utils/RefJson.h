#pragma  once
#include "RefelectionHelper.h"
#include "Log.h"

namespace cppfd {
enum EJsonObjType {
	EJsonObj = 0,
	EJsonArray = 1,
	EJsonMap = 2
};

struct JsonTraveler;
class JsonBase {
  friend bool JsonTravelCallBack(void* pData, const Field& f, const char* szContent, int32_t nContentLen);
  friend int MyTravelLeaf(char top, const char** szJson, JsonTraveler& traveler);
  friend int MyTravelArray(char top, const char** szJson, JsonTraveler& traveler);
 public:
  virtual EJsonObjType GetJsonType() const = 0;
  virtual const std::type_info& GetItemType() const = 0;
  virtual String ToJsonString() const = 0;
  
  virtual bool IsEmpty() const = 0;

  static bool IsBuildInType(const std::type_info& tinfo);

  template<typename T>
  static String ToJsonString(T* pObj) {
    if (pObj == nullptr) return "";
    const Class* pClass = Class::GetClass(typeid(T));
    if (pClass == nullptr) {
      LOG_ERROR("Unknow Type:%s",  Demangle(typeid(T).name()).c_str());
      return "";
    }
    return ToJsonString(pObj, *pClass);
  }

  template<typename T>
  static bool FromJsonString(T* pObj, const String& strJson) {
    if (pObj == nullptr) return false;
    const Class* pClass = Class::GetClass(typeid(T));
    if (pClass == nullptr) {
      LOG_ERROR("Unknow Type:%s", Demangle(typeid(T).name()).c_str());
      return false;
    }
    return FromJsonString(pObj, *pClass, strJson);
  }

  static String ToJsonString(const void* pAddr, const Class& refClass);
  static bool FromJsonString(void* pAddr, const Class& refClass, const String& strJson);

 protected:
  static bool Content2Field(void* pDataAddr, const std::type_info& tinfo, uint32_t nCount, const char* szContent, uint32_t nContentLen);
  static String Field2Json(const void* pData, const std::type_info& tInfo);

  virtual bool AddItemByContent(const char* szContent, uint32_t uLen, const String& strKey = "") = 0;
  virtual void* NewItem() const = 0;
  virtual void AddItem(void* pItem, const String& strKey = "") = 0;
};

template<typename T>
class JsonArray : public JsonBase {
 public:
  JsonArray() : mJsonNewLine(false) { /*static const ReflectiveClass<JsonArray<T> > ref((JsonBase*)nullptr);*/}

  virtual EJsonObjType GetJsonType() const override { return EJsonArray; }
  virtual const std::type_info& GetItemType() const { return typeid(T); }
  virtual String ToJsonString() const override {
    String strRes;
    strRes.append("[");
    size_t isize = mItems.size();
    if (isize != 0 && mJsonNewLine) {
      strRes.append("\n");
    }
    for (size_t i = 0; i < isize; i++) {
      const T& tItem = mItems[i];
      strRes.append(Field2Json(&tItem, typeid(T)));
      if (i != isize - 1) strRes.append(mJsonNewLine ? ",\n" : ",");
    }
    strRes.append("]");
    return strRes;
  }
  
  virtual bool IsEmpty() const override { return mItems.empty(); }
 protected:
  virtual bool AddItemByContent(const char* szContent, uint32_t uLen, const String& strKey = "") override {
    if (!IsBuildInType(typeid(T))) {
      LOG_ERROR("Error Additem for type:%s", Demangle(typeid(T).name()).c_str());
      return false;
    }
    T item;
    Content2Field(&item, typeid(T), 1, szContent, uLen);
    mItems.push_back(item);
    return true;
  }

  virtual void* NewItem() const override { return new T();}
  virtual void AddItem(void* pItem, const String& strKey = "") override { 
    T* pTItem = static_cast<T*>(pItem);
    if (pTItem == nullptr) {
      LOG_ERROR("Error(nullptr) Additem for type:%s", Demangle(typeid(T).name()).c_str());
      return;
    }
    mItems.push_back(*pTItem);
    delete pTItem;
  }
 public:
  std::vector<T> mItems;
  bool mJsonNewLine;
};

template<typename T>
class JsonMap : public JsonBase {
 public:
  JsonMap() { /*static const ReflectiveClass<JsonMap<T> > ref((JsonBase*)nullptr);*/ }
  virtual EJsonObjType GetJsonType() const override { return EJsonMap; }
  virtual const std::type_info& GetItemType() const { return typeid(T); }

  virtual String ToJsonString() const override {
    String strRes;
    strRes.append("{");
    std::size_t nSize = mObjMap.size();
    std::size_t nCounter = 0;
    for (auto it = mObjMap.begin(), itend = mObjMap.end(); it != itend; it++) {
      strRes.append("\"" + it->first + "\":");
      strRes.append(Field2Json(&(it->second), typeid(T)));
      nCounter++;
      if (nCounter < nSize) strRes.append(",");
    }
    strRes.append("}");
    return strRes;
  }

  virtual bool IsEmpty() const override { return mObjMap.empty(); }
 protected:
  virtual bool AddItemByContent(const char* szContent, uint32_t uLen, const String& strKey = "") override {
    if (!IsBuildInType(typeid(T))) {
      LOG_ERROR("Error Additem for type:%s", Demangle(typeid(T).name()).c_str());
      return false;
    }
    if (strKey == "") {
      LOG_ERROR("Empty key while Additem for type:%s", Demangle(typeid(T).name()).c_str());
      return false;
    }
    T& item = mObjMap[strKey];
    Content2Field(&item, typeid(T), 1, szContent, uLen);
    return true;
  }

  virtual void* NewItem() const override { return new T(); }
  virtual void AddItem(void* pItem, const String& strKey = "") override {
    T* pTItem = static_cast<T*>(pItem);
    if (pTItem == nullptr) {
      LOG_ERROR("Error(nullptr) Additem for type:%s", Demangle(typeid(T).name()).c_str());
      return;
    }
    if (strKey == "") {
      LOG_ERROR("Empty key while Additem for type:%s", Demangle(typeid(T).name()).c_str());
      delete pTItem;
      return;
    }
    mObjMap[strKey] = *pTItem;
    delete pTItem;
  }
 public:
  std::map<String, T> mObjMap;
};

}