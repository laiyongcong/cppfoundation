#pragma once
#include "Prerequisites.h"
#include "Utils.h"


#ifndef REF_EXPORT_FLAG
#  define REF_EXPORT
#else
#  define REF_EXPORT struct CPPFD_JOIN(__refflag_, __LINE__) {};
#endif

#define NEWINSTANCE_PTR_CALLER(T)  \
template <typename C, typename... Args> \
static C* __new_instance_ptr__(void* ptr, Args... args) {  \
  return new (ptr) T(args...);  \
} \
template <typename C, typename... Args> \
static C* (*__create_new_instance_ptr__(C* (*)(Args...)))(void*, Args...) { \
  return &__new_instance_ptr__<C, Args...>; \
} \
template <typename C> \
static C* (*__create_new_instance_ptr__(C* (*)()))(void*) { \
  return &__new_instance_ptr__<C>; \
}

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
void ListArgs(TypeList& argList){
  argList.push_back(&typeid(T));
  ListArgs<Args...>(argList);
}

template<typename T>
void ListArgs(TypeList& argList) {
  if (typeid(T) != typeid(void))
    argList.push_back(&typeid(T));
}

struct BaseCallable {
  virtual ~BaseCallable(){}
  virtual int32_t GetArgsCount() const = 0;
  virtual const TypeList& GetArgTypeList() const = 0;
  virtual const std::type_info& GetRetType() const = 0;
};

template <typename... Args>
struct Callable : public BaseCallable {
  const std::type_info& RetType;
  TypeList ArgList;
  Callable(const std::type_info& retType) : RetType(retType) { ListArgs<Args...>(ArgList); }
  int32_t GetArgsCount() const override { return (int32_t)ArgList.size(); }
  virtual const TypeList& GetArgTypeList() const override { return ArgList; }
  virtual const std::type_info& GetRetType() const override { return RetType; }
};

template <typename R, typename... Args>
struct StaticCallable : public Callable<Args...> {
  typedef R (*MethodType)(Args...);
  MethodType method;
  StaticCallable(MethodType m) : Callable<Args...>(typeid(R)), method(m) {}
  R invoke(Args... args) const { return method(args...); }
};

template <typename R>
struct StaticCallable<R> : public Callable<void> {
  typedef R (*MethodType)();
  MethodType method;
  StaticCallable(MethodType m) : Callable<void>(typeid(R)), method(m) {}
  R invoke() const { return method(); }
};

template <typename R, typename C, typename... Args>
struct MemberFuncCallable : public Callable<Args...> {
  typedef R (C::*MethodType)(Args...);
  MethodType method;
  MemberFuncCallable(MethodType m) : Callable<Args...>(typeid(R)), method(m) {}
  R invoke(C* object, Args... args) const { return (object->*method)(args...); }
};

template <typename R, typename C>
struct MemberFuncCallable<R, C> : public Callable<void> {
  typedef R (C::*MethodType)();
  MethodType method;
  MemberFuncCallable(MethodType m) : Callable<void>(typeid(R)), method(m) {}
  R invoke(C* object) const { return (object->*method)(); }
};

template <typename R, typename C, typename... Args>
struct ConstMemberFuncCallable : public Callable<Args...> {
  typedef R (C::*MethodType)(Args...) const;
  MethodType method;
  ConstMemberFuncCallable(MethodType m) : Callable<Args...>(typeid(R)), method(m) {}
  R invoke(C* object, Args... args) const { return (object->*method)(args...); }
};

template <typename R, typename C>
struct ConstMemberFuncCallable<R, C> : public Callable<void> {
  typedef R (C::*MethodType)() const;
  MethodType method;
  ConstMemberFuncCallable(MethodType m) : Callable<void>(typeid(R)), method(m) {}
  R invoke(C* object) const { return (object->*method)(); }
};

template <typename R, typename C, typename... Args>
inline MemberFuncCallable<R, C, Args...>* CreateMemberFuncCallable(R (C::*method)(Args...)) {
  return new MemberFuncCallable<R, C, Args...>(method);
}

template <typename R, typename C>
inline MemberFuncCallable<R, C>* CreateMemberFuncCallable(R (C::*method)()) {
  return new MemberFuncCallable<R, C>(method);
}

template <typename R, typename C, typename... Args>
inline ConstMemberFuncCallable<R, C, Args...>* CreateConstMemberFuncCallable(R (C::*method)(Args...) const) {
  return new ConstMemberFuncCallable<R, C, Args...>(method);
}

template <typename R, typename C>
inline ConstMemberFuncCallable<R, C>* CreateConstMemberFuncCallable(R (C::*method)() const) {
  return new ConstMemberFuncCallable<R, C>(method);
}

template <typename R, typename... Args>
inline StaticCallable<R, Args...>* CreateStaticCallable(R (*method)(Args...)) {
  return new StaticCallable<R, Args...>(method);
}

template <typename R>
inline StaticCallable<R>* CreateStaticCallable(R (*method)()) {
  return new StaticCallable<R>(method);
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
  friend class FieldRegister;

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
   friend class StaticFieldRegister;

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
   void getByIdx(Value& result, int idx = 0) const {
    if (GetAccessType() != AccessPublic) throw IllegalAccessError(GetName());
    if (typeid(Value) != mElementType) throw TypeMismatchError("value");
    if (idx < 0 || idx >= mElementCount) throw IllegalAccessError("idx");
    result = *((Value*)mAddr + idx);
   }

  private:
   StaticField(void* pAddr, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType, const char* strType, const char* strName,
               int nElementCount);
};

class MethodBase : public MemberBase {
   friend class Class;
  public:
   FORCEINLINE const char* GetIdentity() const { return mIdentity.c_str(); }
   FORCEINLINE const char* GetLongIdentity() const { return mLongIdentity.c_str(); }
   FORCEINLINE const char* GetArgs() const { return mArgs.c_str(); }
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
  private:
   std::shared_ptr<BaseCallable> mCallable;
   String mIdentity;
   String mArgs;
   String mLongIdentity;
};

class Method : public MethodBase {
   friend class MethodRegister;
  public:

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

class Class {
  public:
   typedef void* (*NewInstanceFuncPtr)();
   typedef void* (*AllocaInstanceFuncPtr)(void*);
   typedef void* (*SuperCastFuncPtr)(void*);
   typedef const void* (*SuperCastConstFuncPtr)(const void*);

   enum EnumClassType{
     ClassNormalType,
     ClassConstType,
     ClassPointerType,
     ClassConstPointerType,

     ClassTypeNum,
   };

   Class(const char* strName, const Class* super, std::size_t size, NewInstanceFuncPtr newFunc, AllocaInstanceFuncPtr allocaFunc, SuperCastFuncPtr superCastFunc, SuperCastConstFuncPtr superCastConstFunc, 
       const std::type_info& type, const std::type_info& constType, const std::type_info& ptrType, const std::type_info& constPtrType);

   FORCEINLINE int GenFieldIdx() const { return mFieldIdx++; }
   static const Class* GetClassByName(const String& strName);
   static const Class* GetClassByType(const std::type_info& type);
   static std::pair<EnumClassType, const Class*> FindClassByType(const std::type_info& type);

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

   

  private:
   mutable std::atomic_int mFieldIdx;
   String mName;
   String mFullName;
   mutable const Class* mSuper;
   const NewInstanceFuncPtr mNewFunc;
   const AllocaInstanceFuncPtr mAllocaFunc;
   const SuperCastFuncPtr mSuperCastFunc;
   const SuperCastConstFuncPtr mSuperCastConstFunc;
   const std::type_info& mType;
   const std::type_info& mPtrType;

};

}
