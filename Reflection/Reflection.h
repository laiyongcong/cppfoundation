#pragma once
#include "Prerequisites.h"
#include "Utils.h"


#ifndef REF_EXPORT_FLAG
#  define REF_EXPORT
#else
#  define REF_EXPORT struct CPPFD_JOIN(__refflag_, __LINE__) {};
#endif

namespace cppfd {

class Class;

typedef std::vector<const std::type_info*> TypeList;
typedef TypeList ArgumentTypeList;

FORCEINLINE String Demangle(const char* mangledName) {
#ifdef GCC_USEDEMANGLE
  /*  for whatever reason, the ABI::CXA in gcc doesn't do the demangle if it is
  single char name
  v void
  c char
  h unsigned char
  a signed char
  i int
  j unsigned int
  i signed int
  s short
  l long
  x long long
  f float
  d double
  e long double
  */
  if (mangledName[0] != '\0' && mangledName[1] == '\0') {
    switch (mangledName[0]) {
      case 'v':
        return "void";
      case 'c':
        return "char";
      case 'h':
        return "unsigned char";
      case 'a':
        return "signed char";
      case 'i':
        return "int";
      case 'j':
        return "unsigned int";
        //	    case 'i': return "signed int";
      case 's':
        return "short";
      case 't':
        return "unsigned short";
      case 'l':
        return "long";
      case 'm':
        return "unsigned long";
      case 'x':
        return "long long";
      case 'f':
        return "float";
      case 'd':
        return "double";
      case 'e':
        return "long double";
      case 'b':
        return "bool";
      default:
        break;
    }
  }
  String result;
  int status;
  char* unmangled = abi::__cxa_demangle(mangledName, 0, 0, &status);
  if (status == 0) {
    result = String(unmangled);
    free(unmangled);
  } else {
    // Keep mangled name if result is not 0
    result = String(mangledName);
  }
  return result;
#else
  return String(mangledName);
#endif
}

template<typename T, typename... Args>
struct ArgsHelper {
  static void ListArgs(TypeList& argList) {
    if (typeid(T) != typeid(void)) argList.push_back(&typeid(T));
    ArgsHelper<Args...>::ListArgs(argList);
  }
};

template <typename T>
struct ArgsHelper<T> {
  static void ListArgs(TypeList& argList) {
    if (typeid(T) != typeid(void)) argList.push_back(&typeid(T));
  }
};

struct BaseCallable {
  virtual ~BaseCallable(){}
  virtual int32_t GetArgsCount() const = 0;
  virtual const TypeList& GetArgTypeList() const = 0;
  virtual const std::type_info& GetRetType() const = 0;
  virtual String GetArgsString() const = 0;
};

template <typename R, typename... Args>
struct Callable : public BaseCallable {
  TypeList ArgList;
  Callable() { ArgsHelper<Args...>::ListArgs(ArgList); }
  int32_t GetArgsCount() const override { return (int32_t)ArgList.size(); }
  virtual const TypeList& GetArgTypeList() const override { return ArgList; }
  virtual const std::type_info& GetRetType() const override { return typeid(R); }
  virtual String GetArgsString() const override {
    String strRes = "(";
    auto argsList = GetArgTypeList();
    auto it = argsList.begin();
    if (it != argsList.end()) {
      strRes += Demangle((*it)->name());
      it++;
    }
    for (; it != argsList.end(); it++) {
      strRes += ",";
      strRes += Demangle((*it)->name());
    }
    return strRes;
  }
};

template <typename R>
struct Callable<R> : public BaseCallable {
  TypeList ArgList;
  Callable() { ArgsHelper<void>::ListArgs(ArgList); }
  int32_t GetArgsCount() const override { return (int32_t)ArgList.size(); }
  virtual const TypeList& GetArgTypeList() const override { return ArgList; }
  virtual const std::type_info& GetRetType() const override { return typeid(R); }
  virtual String GetArgsString() const override { return "()"; }
};

template <typename R, typename... Args>
struct StaticCallable : public Callable<R, Args...> {
  typedef R (*MethodType)(Args...);
  MethodType method;
  StaticCallable(MethodType m) : Callable<R, Args...>(), method(m) {}
  R invoke(Args... args) const { return method(args...); }
};

template <typename R>
struct StaticCallable<R> : public Callable<R> {
  typedef R (*MethodType)();
  MethodType method;
  StaticCallable(MethodType m) : Callable<R>(), method(m) {}
  R invoke() const { return method(); }
};

template <typename R, typename C, typename... Args>
struct MemberFuncCallable : public Callable<R, Args...> {
  typedef R (C::*MethodType)(Args...);
  MethodType method;
  MemberFuncCallable(MethodType m) : Callable<R, Args...>(), method(m) {}
  R invoke(C* object, Args... args) const { return (object->*method)(args...); }
};

template <typename R, typename C>
struct MemberFuncCallable<R, C> : public Callable<R> {
  typedef R (C::*MethodType)();
  MethodType method;
  MemberFuncCallable(MethodType m) : Callable<R>(), method(m) {}
  R invoke(C* object) const { return (object->*method)(); }
};

template <typename R, typename C, typename... Args>
struct ConstMemberFuncCallable : public Callable<R, Args...> {
  typedef R (C::*MethodType)(Args...) const;
  MethodType method;
  ConstMemberFuncCallable(MethodType m) : Callable<R, Args...>(), method(m) {}
  R invoke(C* object, Args... args) const { return (object->*method)(args...); }
};

template <typename R, typename C>
struct ConstMemberFuncCallable<R, C> : public Callable<R> {
  typedef R (C::*MethodType)() const;
  MethodType method;
  ConstMemberFuncCallable(MethodType m) : Callable<R>(), method(m) {}
  R invoke(C* object) const { return (object->*method)(); }
};

template <typename R, typename C, typename... Args>
inline std::shared_ptr<BaseCallable> CreateMemberFuncCallable(R (C::*method)(Args...)) {
  return std::shared_ptr<BaseCallable>(new MemberFuncCallable<R, C, Args...>(method));
}

template <typename R, typename C>
inline std::shared_ptr<BaseCallable> CreateMemberFuncCallable(R (C::*method)()) {
  return std::shared_ptr<BaseCallable>(new MemberFuncCallable<R, C>(method));
}

template <typename R, typename C, typename... Args>
inline std::shared_ptr<BaseCallable> CreateConstMemberFuncCallable(R (C::*method)(Args...) const) {
  return std::shared_ptr<BaseCallable>(new ConstMemberFuncCallable<R, C, Args...>(method));
}

template <typename R, typename C>
inline std::shared_ptr<BaseCallable> CreateConstMemberFuncCallable(R (C::*method)() const) {
  return std::shared_ptr<BaseCallable>(new ConstMemberFuncCallable<R, C>(method));
}

template <typename R, typename... Args>
inline std::shared_ptr<BaseCallable> CreateStaticCallable(R (*method)(Args...)) {
  return std::shared_ptr<BaseCallable>(new StaticCallable<R, Args...>(method));
}

template <typename R>
inline std::shared_ptr<BaseCallable> CreateStaticCallable(R (*method)()) {
  return std::shared_ptr<BaseCallable> (new StaticCallable<R>(method));
}

enum EnumAccessType {
    AccessPublic,
    AccessProtected,
    AccessPrivate
};

class TypeMismatchError : public std::runtime_error {
 public:
  TypeMismatchError(const String& what) throw() : runtime_error(what.c_str()) { Utils::ExeceptionLog(what); }
};

class IllegalAccessError : public std::runtime_error {
 public:
  IllegalAccessError(const String& what) throw() : runtime_error(what.c_str()) { Utils::ExeceptionLog(what); }
};

class UnknownFieldError : public std::runtime_error {
 public:
  UnknownFieldError(const String& what) throw() : runtime_error(what.c_str()) { Utils::ExeceptionLog(what); }
};

class UnknownMethodError : public std::runtime_error {
 public:
  UnknownMethodError(const String& what) throw() : runtime_error(what.c_str()) { Utils::ExeceptionLog(what); }
};

class SuperClassError : public std::runtime_error {
 public:
  SuperClassError(const String& what) throw() : runtime_error(what.c_str()) { Utils::ExeceptionLog(what); }
};

class ClassRegistrationError : public std::runtime_error {
 public:
  ClassRegistrationError(const String& what) throw() : runtime_error(what.c_str()) { Utils::ExeceptionLog(what); }
};

class InvalidStateError : public std::runtime_error {
 public:
  InvalidStateError(const String& what) throw() : runtime_error(what.c_str()) { Utils::ExeceptionLog(what); }
};

class MemberBase : public NonCopyable {
 public:
  virtual ~MemberBase(){}
  FORCEINLINE const Class& GetClass() const { return *mClass; }
  FORCEINLINE const EnumAccessType GetAccessType() const { return mAccessType; }
  FORCEINLINE const char* GetType() const { return mType.c_str(); }
  FORCEINLINE const char* GetName() const { return mName.c_str(); }
  FORCEINLINE void SetRelevantDataPtr(intptr_t DataPtr) { mRelevantDataPtr = DataPtr; }
  FORCEINLINE intptr_t GetRelevantDataPtr() const { return mRelevantDataPtr; }

 protected:
  MemberBase(const Class* pclass, EnumAccessType access, const char* type, const char* name);
  MemberBase& operator=(const MemberBase& other);
  MemberBase(const MemberBase& other);

 private:
  const Class* mClass;
  String mType;
  String mName;
  intptr_t mRelevantDataPtr;
 protected:
  EnumAccessType mAccessType;
};

class Field : public MemberBase {
  friend class Class;
  friend struct FieldRegister;

 private:
  const std::type_info& mType;
  const std::type_info& mElementType; //for ArrayType
  uint64_t mOffset;
  std::size_t mSize;
  int mElementCount; //for array type
  int mFieldIdx;     //field index in Class

  public:
   FORCEINLINE const std::type_info& GetType() const { return mType; }
   FORCEINLINE const std::type_info& GetElementType() const { return mElementType; }
   FORCEINLINE std::size_t GetSize() const { return mSize; }
   FORCEINLINE bool IsArray() const { return mType != mElementType; }
   FORCEINLINE int GetElementCount() const { return mElementCount; }
   FORCEINLINE int GetFieldIdx() const { return mFieldIdx; }
   FORCEINLINE uint64_t GetOffset() const { return mOffset; }

  private:
   Field(uint64_t offset, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType, const char* strType, const char* strName,
         int nElementCount);
};

class StaticField : public MemberBase {
   friend class Class;
   friend struct StaticFieldRegister;

  private:
   const std::type_info& mType;
   const std::type_info& mElementType;  // for ArrayType
   void* mAddr;
   std::size_t mSize;
   int mElementCount;  // for array type

  public:
   FORCEINLINE const std::type_info& GetType() const { return mType; }
   FORCEINLINE const std::type_info& GetElementType() const { return mElementType; }
   FORCEINLINE std::size_t GetSize() const { return mSize; }
   FORCEINLINE bool IsArray() const { return mType != mElementType; }
   FORCEINLINE int GetElementCount() const { return mElementCount; }

   template <typename Value>
   void Get(Value& value) const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetName());
    if (typeid(Value) != mType) throw TypeMismatchError("value");
    ::memcpy(&value, mAddr, mSize);
   }

   template <typename Object, typename Value>
   void GetByIdx(Value& result, int idx = 0) const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetName());
    if (typeid(Value) != mElementType) throw TypeMismatchError("value");
    if (idx < 0 || idx >= mElementCount) throw IllegalAccessError("idx");
    result = *((Value*)mAddr + idx);
   }

  private:
   StaticField(void* pAddr, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType, const char* strType, const char* strName,
               int nElementCount = 1);
};

class MethodBase : public MemberBase {
   friend class Class;
  public:
   FORCEINLINE const String& GetIdentity() const { return mIdentity; }
   FORCEINLINE const String& GetLongIdentity() const { return mLongIdentity; }
   FORCEINLINE const String& GetArgs() const { return mArgs; }
   FORCEINLINE const ArgumentTypeList& GetArgsTypeList() const {
    if (nullptr == mCallable.get()) throw InvalidStateError(String(GetName()) + " not initialized");
    return mCallable->GetArgTypeList();
   }
   FORCEINLINE int32_t GetArgsCount() const {
    if (nullptr == mCallable.get()) throw InvalidStateError(String(GetName()) + " not initialized");
    return mCallable->GetArgsCount();
   }
   FORCEINLINE const std::type_info& GetReturnType() const {
    if (nullptr == mCallable.get()) throw InvalidStateError(String(GetName()) + " not initialized");
    return mCallable->GetRetType();
   }
   virtual String GetPrefix() const;

  protected:
   template <typename... Args>
   String FindMisMatchedInfo(const std::type_info& retType, const std::type_info& classType) const;

   template <>
   String FindMisMatchedInfo(const std::type_info& retType, const std::type_info& classType) const;

   template <typename... Args>
   bool TestCompatible(const std::type_info& retType, const std::type_info& classType, void* objPtr) const;

   template <>
   bool TestCompatible(const std::type_info& retType, const std::type_info& classType, void* objPtr) const;

  protected:
   MethodBase(const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs, std::shared_ptr<BaseCallable> cb);

   std::shared_ptr<BaseCallable> mCallable;

  private:
   String mIdentity;
   String mArgs;
   String mLongIdentity;
};

class Method : public MethodBase {
   friend struct MethodRegister;
   friend class Class;
  public:
   FORCEINLINE bool IsVirtual() const { return mIsVirtual; }
  private:
   bool mIsVirtual;

  public:
   template <typename R, typename C, typename... Args>
   R Invoke(C* object, Args... args) const;

   template <typename R, typename C>
   R Invoke(C* object) const;

   template <typename C, typename... Args>
   void Invoke(C* object, Args... args) const;

   template <typename C>
   void Invoke(C* object) const;

  private:
   Method(const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs, bool bVirtual, std::shared_ptr<BaseCallable> cb);
};

class StaticMethod : public MethodBase {
   friend struct StaticMethodRegister;
   friend class Class;

  public:
   template <typename R, typename... Args>
   R Invoke(Args... args) const;

   template <typename R>
   R Invoke() const;

   template <typename... Args>
   void Invoke(Args... args) const;

   template <>
   void Invoke() const;

   virtual String GetPrefix() const override { return "static " + MethodBase::GetPrefix(); }

  protected:
   StaticMethod(const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs, std::shared_ptr<BaseCallable> cb) 
       : MethodBase(pClass, accessType, szType, szName, szArgs, cb){}
};

class ConstructorMethod : public StaticMethod {
   friend class Class;
   friend struct ConstructorMethodRegistor;

  public:
   virtual String GetPrefix() const override;

   template <typename R, typename... Args>
   R Alloca(void* ptr, Args... args) {
    if (mPlacementCallable == 0) throw UnknownMethodError("placement constructor");
    return this->InvokeAlloca(ptr, args...);
   }

   template <typename R>
   R Alloca(void* ptr) {
    if (mPlacementCallable == 0) throw UnknownMethodError("placement constructor");
    return this->InvokeAlloca<R, void*>(ptr);
   }
   
  private:
   std::shared_ptr<BaseCallable> mPlacementCallable;

  private:
   ConstructorMethod(const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs, std::shared_ptr<BaseCallable> cb, std::shared_ptr<BaseCallable> placementCb)
       : StaticMethod(pClass, accessType, szType, szName, szArgs, cb), mPlacementCallable(placementCb) {}

   template <typename R, typename... Args>
   R InvokeAlloca(Args... args);
};



class TypeInfo {
  public:
   FORCEINLINE TypeInfo() : pInfo(0) {}

   FORCEINLINE TypeInfo(const std::type_info& tinfo) : pInfo(&tinfo) {}

   FORCEINLINE TypeInfo(const TypeInfo& that) : pInfo(that.pInfo) {}

   FORCEINLINE TypeInfo& operator=(const TypeInfo& that) {
    this->pInfo = that.pInfo;
    return *this;
   }

   FORCEINLINE const std::type_info& get() const {
    if (this->pInfo == 0) throw InvalidStateError("TypeInfo is not initialized");
    return *this->pInfo;
   }

   FORCEINLINE const std::type_info& operator*() const { return this->get(); }

   FORCEINLINE bool before(const TypeInfo& that) const { return this->get().before(that.get()); }

   FORCEINLINE const char* name() const { return this->get().name(); }

  private:
   const std::type_info* pInfo;
};

FORCEINLINE bool operator==(const TypeInfo& a, const TypeInfo& b) { return a.get() == b.get(); }

FORCEINLINE bool operator!=(const TypeInfo& a, const TypeInfo& b) { return a.get() != b.get(); }

FORCEINLINE bool operator<(const TypeInfo& a, const TypeInfo& b) { return a.before(b); }

FORCEINLINE bool operator<=(const TypeInfo& a, const TypeInfo& b) { return a == b || a.before(b); }

FORCEINLINE bool operator>(const TypeInfo& a, const TypeInfo& b) { return !(a <= b); }

FORCEINLINE bool operator>=(const TypeInfo& a, const TypeInfo& b) { return !(a < b); }

typedef std::map<String, const Class*> ClassMap;

typedef std::vector<const Class*> ClassList;

typedef std::map<TypeInfo, const Class*> TypeInfoMap;

typedef std::vector<const Field*> FieldList;

typedef std::map<String, const Field*> FieldMap;

typedef std::vector<const StaticField*> StaticFieldList;

typedef std::map<String, const StaticField*> StaticFieldMap;

typedef std::vector<const Method*> MethodList;

typedef std::map<String, const Method*> MethodMap;

typedef std::vector<const StaticMethod*> StaticMethodList;

typedef std::map<String, const StaticMethod*> StaticMethodMap;

typedef std::vector<const ConstructorMethod*> ConstructorList;

class Class {
   friend struct FieldRegister;
   friend struct StaticFieldRegister;
   friend struct MethodRegister;
   friend struct StaticMethodRegister;
   friend struct ConstructorMethodRegistor;
  public:
   typedef void* (*SuperCastFuncPtr)(void*);
   typedef const void* (*SuperCastConstFuncPtr)(const void*);

   enum EnumClassType{
     ClassNormalType,
     ClassConstType,
     ClassPointerType,
     ClassConstPointerType,

     ClassTypeNum,
   };

   Class(const char* strName, const Class* super, std::size_t size, SuperCastFuncPtr superCastFunc, SuperCastConstFuncPtr superCastConstFunc, 
       const std::type_info& type, const std::type_info& constType, const std::type_info& ptrType, const std::type_info& constPtrType);
   virtual ~Class();

   FORCEINLINE int GenFieldIdx() const { return mFieldIdx++; }
   static const Class* GetClassByName(const String& strName);
   static const Class* GetClassByType(const std::type_info& type);
   static std::pair<EnumClassType, const Class*> FindClassByType(const std::type_info& type);
   static bool IsCastable(const std::type_info& from_cls, const std::type_info& to_cls, void* objptr = 0);
   static bool ArgsSame(const ArgumentTypeList& argsList1, const ArgumentTypeList& argsList2);

   const Field* GetField(const char* szName, bool bSearchSuper = true) const;
   const StaticField* GetStaticField(const char* szName, bool bSearchSuper = true) const;
   const Method* GetMethod(const char* szName, bool bSearchSuper = true) const;
   const StaticMethod* GetStaticMethod(const char* szName, bool bSearchSuper = true) const;

  private:
   static ClassMap& GetClassMap();
   static TypeInfoMap& GetTypeInfoMap(EnumClassType eType);

   FORCEINLINE void RegistClassMap(const String& strName) { GetClassMap().insert(std::make_pair(strName, this)); }
   FORCEINLINE void RegistClassTypeMap(EnumClassType eType, const std::type_info& type) { GetTypeInfoMap(eType).insert(std::make_pair(TypeInfo(type), this)); }

  public:
   FORCEINLINE const String& GetName() const { return mName; }
   FORCEINLINE const String& GetFullName() const { return mFullName; }
   FORCEINLINE const Class* Super() const { return mSuper; }
   FORCEINLINE const std::type_info& GetTypeInfo() const { return mType; }
   FORCEINLINE const std::type_info& GetPtrTypeInfo() const { return mPtrType; }
   FORCEINLINE void* DynamicCastFromSuper(void* p) const { return mSuperCastFunc(p); }
   FORCEINLINE const void* DynamicCastFromSuperConst(const void* p) const { return mSuperCastConstFunc(p); }
   bool DynamicCastableFromSuper(void* pSuper, const Class* pSuperClass) const;
   bool DynamicCastableFromSuper(const void* pSuper, const Class* pSuperClass) const;
   bool IsSuperOf(const Class& cl) const;
   FORCEINLINE bool IsSameOrSuperOf(const Class& cl) const { return IsSame(cl) || IsSuperOf(cl); }
   FORCEINLINE  bool IsRelative(const Class& cl) const { return IsSame(cl) || IsSuperOf(cl) || cl.IsSuperOf(*this); }
   FORCEINLINE bool IsSame(const Class& cl) const { return this == &cl; }
   
   template<typename R, typename ...Args>
   R* NewInstance(Args... args) const {
    typedef const StaticCallable<R*, Args...> NewInstanceCallable;
    ArgumentTypeList argsList;
    ArgsHelper<Args...>::ListArgs(argsList);
    for (auto it = mConstructors.begin(); it != mConstructors.end(); it++)
    {
      const ConstructorMethod& method = **it;
      if (ArgsSame(argsList, method.GetArgsTypeList())) {
        NewInstanceCallable* newCb = dynamic_cast<NewInstanceCallable*>(method.mCallable);
        if (newCb && method.GetAccessType() == AccessPublic) {
          return newCb->invoke(args...);
        }
      }
    }
    return nullptr;
   }

   template <typename R>
   R* NewInstance() const {
    typedef const StaticCallable<R*> NewInstanceCallable;
    for (auto it = mConstructors.begin(); it != mConstructors.end(); it++) {
      const ConstructorMethod& method = **it;
      if (method.GetArgsCount() == 0) {
        NewInstanceCallable* newCb = dynamic_cast<NewInstanceCallable*>(method.mCallable);
        if (newCb && method.GetAccessType() == AccessPublic) {
          return newCb->invoke();
        }
      }
    }
    return nullptr;
   }

   template <typename R, typename... Args>
   R* Alloca(void* ptr, Args... args) const {
    typedef const StaticCallable<R*, void*, Args...> AllocaInstanceCallable;
    ArgumentTypeList argsList;
    ArgsHelper<Args...>::ListArgs(argsList);
    for (auto it = mConstructors.begin(); it != mConstructors.end(); it++) {
      const ConstructorMethod& method = **it;
      if (method.mPlacementCallable.get() && ArgsSame(argsList, method.GetArgsTypeList())) {
        NewInstanceCallable* allocaCb = dynamic_cast<AllocaInstanceCallable*>(method.mPlacementCallable);
        if (allocaCb && method.GetAccessType() == AccessPublic) {
          return method.Alloca<R*>(ptr, args...);
        }
      }
    }
    return nullptr;
   }

   template <typename R>
   R* Alloca(void* ptr) const {
    typedef const StaticCallable<R*, void*> AllocaInstanceCallable;
    for (auto it = mConstructors.begin(); it != mConstructors.end(); it++) {
      const ConstructorMethod& method = **it;
      if (method.mPlacementCallable.get() && method.mPlacementCallable->GetArgsCount() == 0) {
        NewInstanceCallable* allocaCb = dynamic_cast<AllocaInstanceCallable*>(method.mPlacementCallable);
        if (allocaCb && method.GetAccessType() == AccessPublic) {
          return method.Alloca<R*>(ptr);
        }
      }
    }
    return nullptr;
   }

  protected:
   void AddField(Field* pField);
   void AddStaticField(StaticField* pField);
   void AddMethod(Method* pMethod);
   void AddStaticMethod(StaticMethod* pMethod);
   void AddConstructorMethod(ConstructorMethod* pMethod);
  private:
   mutable std::atomic_int mFieldIdx;
   String mName;
   String mFullName;
   mutable const Class* mSuper;
   const SuperCastFuncPtr mSuperCastFunc;
   const SuperCastConstFuncPtr mSuperCastConstFunc;
   const std::type_info& mType;
   const std::type_info& mPtrType;

   FieldList mFields;
   FieldMap mFieldMap;
   StaticFieldList mStaticFieldList;
   StaticFieldMap mStaticFieldMap;
   MethodList mMethods;
   MethodMap mMethodMap;
   StaticMethodList mStaticMethods;
   StaticMethodMap mStaticMethodMap;
   ConstructorList mConstructors;
};

template<typename... Args>
String MethodBase::FindMisMatchedInfo(const std::type_info& retType, const std::type_info& classType) const {
   oStringStream info;
   if (GetClass().GetTypeInfo() != classType) {
    if (Class::IsCastable(GetClass().GetTypeInfo(), classType)) info << "(WARN only: type castable)";
    info << "object type mismatched, expected:" << Demangle(GetClass().GetTypeInfo().name()) << " passed:" << Demangle(classType.name());
   }
   std::size_t passedArgLen = sizeof...(Args);
   if (GetArgsCount() != passedArgLen) {
    info << "mismatched number of parameter, expected:" << GetArgsCount() << " passed:" << passedArgLen;
   }
   if (retType != GetReturnType()) {
    if (Class::IsCastable(GetReturnType(), retType)) info << "(WARN only: type castable)";
    info << "return type mismatched, expected:" << Demangle(GetReturnType().name()) << " passed:" << Demangle(retType.name());
   }
   ArgumentTypeList argList;
   ArgsHelper<Args...>::ListArgs(argList);
   int idx = 1;
   for (auto itPassed = argList.begin(), auto itExp = GetArgsTypeList().begin(); itPassed != argList.end();) {
    if (**itPassed != **itExp) {
    info << "(WARN only: type castable)";
    info << "parameter " << idx << " mismatched, expected:" << Demangle((*itExp)->name()) << " passed:" << Demangle((*itPassed)->name()) << "\n";
    }
    itPassed++;
    itExp++;
    idx++;
   }
   return String(info.str());
}

template <>
String MethodBase::FindMisMatchedInfo(const std::type_info& retType, const std::type_info& classType) const {
   oStringStream info;
   if (GetClass().GetTypeInfo() != classType) {
    if (Class::IsCastable(GetClass().GetTypeInfo(), classType)) info << "(WARN only: type castable)";
    info << "object type mismatched, expected:" << Demangle(GetClass().GetTypeInfo().name()) << " passed:" << Demangle(classType.name());
   }
   if (GetArgsCount() != 0) {
    info << "mismatched number of parameter, expected:" << GetArgsCount() << " passed:0";
   }
   if (retType != GetReturnType()) {
    if (Class::IsCastable(GetReturnType(), retType)) info << "(WARN only: type castable)";
    info << "return type mismatched, expected:" << Demangle(GetReturnType().name()) << " passed:" << Demangle(retType.name());
   }
   return String(info.str());
}

template<typename... Args>
bool MethodBase::TestCompatible(const std::type_info& retType, const std::type_info& classType, void* objPtr) const {
   if (!Class::IsCastable(GetClass().GetTypeInfo(), classType, objPtr) || !Class::IsCastable(GetReturnType(), retType)) {
    return false;
   }
   ArgumentTypeList argList;
   ArgsHelper<Args...>::ListArgs(argList);
   if (GetArgsCount() != (int32_t)argList.size()) {
    return false;
   }
   for (auto itPassed = argList.begin(), auto itExp = GetArgsTypeList().begin(); itPassed != argList.end();) {
    if (!Class::IsCastable(**itExp, **itPassed)) return false;
    itPassed++;
    itExp++;
   }
   return true;
}

template <>
bool MethodBase::TestCompatible(const std::type_info& retType, const std::type_info& classType, void* objPtr) const {
   if (!Class::IsCastable(GetClass().GetTypeInfo(), classType, objPtr) || !Class::IsCastable(GetReturnType(), retType)) {
    return false;
   }
   if (GetArgsCount() != 0) return false;
   return true;
}

template <typename R, typename C, typename... Args>
R Method::Invoke(C* object, Args... args) const {
   if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
   typedef const MemberFuncCallable<R, C, Args...> CallableType;
   typedef const ConstMemberFuncCallable<R, C, Args...> ConstCallableType;
   CallableType* cb = dynamic_cast<CallableType*>(mCallable.get());
   if (cb) {
     return cb->invoke(object, args...);
   }
   ConstCallableType* constCb = dynamic_cast<ConstCallableType*>(mCallable.get());
   if (constCb) {
     return constCb->invoke(object, args...);
   }
   if (TestCompatible<Args...>(typeid(R), typeid(C*), object)) {
     cb = (CallableType*)(mCallable.get());
     return cb->invoke(object, args...);
   }
   throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo<Args...>(typeid(R), typeid(C*)));
}

template <typename R, typename C>
R Method::Invoke(C* object) const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
    typedef const MemberFuncCallable<R, C> CallableType;
    typedef const ConstMemberFuncCallable<R, C> ConstCallableType;
    CallableType* cb = dynamic_cast<CallableType*>(mCallable.get());
    if (cb) {
      return cb->invoke(object);
    }
    ConstCallableType* constCb = dynamic_cast<ConstCallableType*>(mCallable.get());
    if (constCb) {
      return constCb->invoke(object);
    }
    if (TestCompatible(typeid(R), typeid(C*), object)) {
      cb = (CallableType*)(mCallable.get());
      return cb->invoke(object);
    }
    throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo(typeid(R), typeid(C*)));
}

template <typename C, typename... Args>
void Method::Invoke(C* object, Args... args) const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
    typedef const MemberFuncCallable<void, C, Args...> CallableType;
    typedef const ConstMemberFuncCallable<void, C, Args...> ConstCallableType;
    CallableType* cb = dynamic_cast<CallableType*>(mCallable.get());
    if (cb) {
      cb->invoke(object, args...);
    }
    ConstCallableType* constCb = dynamic_cast<ConstCallableType*>(mCallable.get());
    if (constCb) {
      constCb->invoke(object, args...);
    }
    if (TestCompatible<Args...>(typeid(void), typeid(C*), object)) {
      cb = (CallableType*)(mCallable.get());
      cb->invoke(object, args...);
    }
    throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo<Args...>(typeid(void), typeid(C*)));
}

template <typename C>
void Method::Invoke(C* object) const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
    typedef const MemberFuncCallable<void, C> CallableType;
    typedef const ConstMemberFuncCallable<void, C> ConstCallableType;
    CallableType* cb = dynamic_cast<CallableType*>(mCallable.get());
    if (cb) {
      cb->invoke(object);
    }
    ConstCallableType* constCb = dynamic_cast<ConstCallableType*>(mCallable.get());
    if (constCb) {
      constCb->invoke(object);
    }
    if (TestCompatible(typeid(void), typeid(C*), object)) {
      cb = (CallableType*)(mCallable.get());
      cb->invoke(object);
    }
    throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo(typeid(void), typeid(C*)));
}

template <typename R, typename... Args>
R StaticMethod::Invoke(Args... args) const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
    typedef const StaticCallable<R, Args...> CallableType;
    CallableType* cb = dynamic_cast<CallableType*>(mCallable.get());
    if (cb) {
      return cb->invoke(args...);
    }
    if (TestCompatible<Args...>(typeid(R), GetClass().GetTypeInfo(), nullptr)) {
      cb = (CallableType*)(mCallable.get());
      return cb->invoke(args...);
    }
    throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo<Args...>(typeid(R), GetClass().GetTypeInfo()));
}

template <typename R>
R StaticMethod::Invoke() const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
    typedef const StaticCallable<R> CallableType;
    CallableType* cb = dynamic_cast<CallableType*>(mCallable.get());
    if (cb) {
      return cb->invoke();
    }
    if (TestCompatible(typeid(R), GetClass().GetTypeInfo(), nullptr)) {
      cb = (CallableType*)(mCallable.get());
      return cb->invoke();
    }
    throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo(typeid(R), GetClass().GetTypeInfo()));
}

template <typename... Args>
void StaticMethod::Invoke(Args... args) const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
    typedef const StaticCallable<void, Args...> CallableType;
    CallableType* cb = dynamic_cast<CallableType*>(mCallable.get());
    if (cb) {
      cb->invoke(args...);
    }
    if (TestCompatible<Args...>(typeid(void), GetClass().GetTypeInfo(), nullptr)) {
      cb = (CallableType*)(mCallable.get());
      cb->invoke(args...);
    }
    throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo<Args...>(typeid(void), GetClass().GetTypeInfo()));
}

template <>
void StaticMethod::Invoke() const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
    typedef const StaticCallable<void> CallableType;
    CallableType* cb = dynamic_cast<CallableType*>(mCallable.get());
    if (cb) {
      cb->invoke();
    }
    if (TestCompatible(typeid(void), GetClass().GetTypeInfo(), nullptr)) {
      cb = (CallableType*)(mCallable.get());
      cb->invoke();
    }
    throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo(typeid(void), GetClass().GetTypeInfo()));
}

template <typename R, typename... Args>
R ConstructorMethod::InvokeAlloca(Args... args) {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetIdentity());
    typedef StaticCallable<R, Args...> CallableType;
    CallableType* cb = dynamic_cast<CallableType*>(mPlacementCallable);
    if (cb) {
      return cb->invoke(args...);
    }
    if (TestCompatible()<Args...>(mPlacementCallable, GetClass().GetTypeInfo(), typeid(R), GetClass().GetTypeInfo(), 0)) {
      cb = (CallableType*)mPlacementCallable;
      return cb->invoke(args...);
    }
    throw TypeMismatchError(GetLongIdentity() + ":\n" + FindMisMatchedInfo<Args...>(typeid(R), GetClass().GetTypeInfo()));
}

struct FieldRegister {
    FieldRegister(uint64_t offset, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType, const char* szType,
                  const char* szName, int nCount);
};
struct StaticFieldRegister {
    StaticFieldRegister(void* pAddr, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType, const char* szType,
                        const char* szName, int nCount);
};
struct MethodRegister {
    MethodRegister(std::shared_ptr<BaseCallable> pCb, const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs, bool bVirtual);
};
struct StaticMethodRegister {
    StaticMethodRegister(std::shared_ptr<BaseCallable> pCb, const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs);
};
struct ConstructorMethodRegistor {
    ConstructorMethodRegistor(std::shared_ptr<BaseCallable> pCb, std::shared_ptr<BaseCallable> pPlacementCb, const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName,
                              const char* szArgs);
};
}
