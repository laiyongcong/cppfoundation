#pragma once
#include "Reflection.h"

namespace cppfd {
template <typename T>
class ReflectiveClass {
  struct FieldInfo {
    const std::type_info& mElementType;
    int mElementCount;
    FieldInfo(int nCount, const std::type_info& type) : mElementType(type), mElementCount(nCount){}
  };

  public:
   ReflectiveClass() {
     InitClass(Demangle(typeid(T)), nullptr, &NullSuperCast, &NullSuperCastConst);
   }

   ReflectiveClass(const char* szName) {
     InitClass(szName, nullptr, &NullSuperCast, &NullSuperCastConst);
   }

   template<typename SuperT>
   ReflectiveClass(const SuperT*) {
     const Class* pClass = Class::GetClassByType(typeid(SuperT));
     InitClass(Demangle(typeid(T)), pClass, &SuperCast<SuperT>, &SuperCastConst<SuperT>);
   }

   template <typename SuperT>
   ReflectiveClass(const char* szName, const SuperT*) {
     const Class* pClass = Class::GetClassByType(typeid(SuperT));
     InitClass(szName, pClass, &SuperCast<SuperT>, &SuperCastConst<SuperT>);
   }

   template<typename F>
   ReflectiveClass& RefField(const F T::* fieldptr, const char* szName, EnumAccessType accType = AccessPublic) {
     static char szTmpBuff[sizeof(F)];
     F* pField = (F*)szTmpBuff;
     FieldInfo fInfo = GetFieldInfo(*pField);

     unsigned long long uOffset = ((unsigned long long)&(((const T*)1024)->*fieldptr)) - 1024;
     FieldRegister regist((uint64_t)uOffset, sizeof(F), typeid(F), fInfo.mElementType, mClass.get(), accType, Demangle(typeid(F).name()).c_str(), szName, fInfo.mElementCount);
     return *this;
   }

   template <typename F>
   ReflectiveClass& RefStaticField(const F *fieldptr, const char* szName, EnumAccessType accType = AccessPublic) {
     static char szTmpBuff[sizeof(F)];
     F* pField = (F*)szTmpBuff;
     FieldInfo fInfo = GetFieldInfo(*pField);

     StaticFieldRegister regist(const_cast<F*>(fieldptr), sizeof(F), typeid(F), fInfo.mElementType, mClass.get(), accType, Demangle(typeid(F).name()).c_str(), szName, fInfo.mElementCount);
     return *this;
   }

   template <typename M>
   ReflectiveClass& RefMethod(M methodPtr, const char* szName, bool bVirtual = false, EnumAccessType accType = AccessPublic) {
     auto pCb = CreateMemberFuncCallable(methodPtr);
     MethodRegister reigst(pCb, mClass.get(), accType, Demangle(pCb->GetRetType().name()).c_str(), szName, pCb->GetArgsString().c_str(), bVirtual);
     return *this;
   }

   template<typename M>
   ReflectiveClass& RefConstMethod(M methodPtr, const char* szName, bool bVirtual = false, EnumAccessType accType = AccessPublic) {
     auto pCb = CreateConstMemberFuncCallable(methodPtr);
     MethodRegister reigst(pCb, mClass.get(), accType, Demangle(pCb->GetRetType().name()).c_str(), szName, pCb->GetArgsString().c_str(), bVirtual);
     return *this;
   }

   template <typename M>
   ReflectiveClass& RefStaticMethod(M methodPtr, const char* szName, bool bVirtual = false, EnumAccessType accType = AccessPublic) {
     auto pCb = CreateStaticCallable(methodPtr);
     StaticMethodRegister reigst(pCb, mClass.get(), accType, Demangle(pCb->GetRetType().name()).c_str(), szName, pCb->GetArgsString().c_str());
     return *this;
   }

   template<typename... Args>
   ReflectiveClass& RefConstructor() {
     auto pConstructorCallable = CreateStaticCallable<T*, Args...>(&NewInstance<T, Args...>);
     auto pAllocaCallable = CreateStaticCallable<T*, void*, Args...>(CreateAllocaFuncPtr<T, Args...>(&NewInstance<T, Args...>));
     ConstructorMethodRegistor reigst(pConstructorCallable, pAllocaCallable, mClass.get(), AccessPublic, mClass->GetName().c_str(), mClass->GetName().c_str(), pConstructorCallable->GetArgsString().c_str());
     return *this;
   }

   template <>
   ReflectiveClass& RefConstructor() {
     auto pConstructorCallable = CreateStaticCallable<T*>(&NewInstance<T>);
     auto pAllocaCallable = CreateStaticCallable<T*>(CreateAllocaFuncPtr<T>(&NewInstance<T>));
     ConstructorMethodRegistor reigst(pConstructorCallable, pAllocaCallable, mClass.get(), AccessPublic, mClass->GetName().c_str(), mClass->GetName().c_str(), pConstructorCallable->GetArgsString().c_str());
     return *this;
   }

  private:
   template <typename C, typename... Args>
   static C* Allocate(void* ptr, Args... args) { return new (ptr) C(args...); }

   template <typename C, typename... Args>
   static C* (*CreateAllocaFuncPtr(C* (*)(Args...)))(void*, Args...) {
     return &Allocate<C, Args...>;
   }

   template <typename C>
   static C* (*CreateAllocaFuncPtr(C* (*)()))(void*) {
     return &Allocate<C>;
   }

   static void* NullSuperCast(void* p) { return nullptr; }

   static const void* NullSuperCastConst(const void* p) { return nullptr; }

   template <typename SuperT>
   static void* SuperCast(void* p) {
    return dynamic_cast<T*>(static_cast<SuperT*>(p));
   }

   template <typename SuperT>
   static const void* SuperCastConst(const void* p) {
    return dynamic_cast<const T*>(static_cast<const SuperT*>(p));
   }

   template <typename C, typename... Args>
   static C* NewInstance(Args... args) {
    return new C(args...);
   }

   template <typename C>
   static C* NewInstance() {
    return new C();
   }

   void InitClass(const char* szName, const Class* pSuper, Class::SuperCastFuncPtr superCast, Class::SuperCastConstFuncPtr superCastConst) {
    mClass.reset(new Class(szName, pSuper, sizeof(T), superCast, superCastConst, typeid(T), typeid(const T), typeid(T*), typeid(const T*)));
   }

   template <typename F, int N>
   FORCEINLINE FieldInfo GetFieldInfo(F (&c)[N]) {
     return std::move(FieldInfo(N, typeid(F)));
   }
   template <typename F>
   FORCEINLINE FieldInfo GetFieldInfo(F(&c)) {
     return std::move(FieldInfo(1, typeid(F)));
   }

  private:
   std::shared_ptr<Class> mClass;
};
}