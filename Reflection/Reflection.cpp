#include "Reflection.h"

namespace cppfd{

 MemberBase::MemberBase(const Class* pclass, EnumAccessType access, const char* type, const char* name) : mClass(pclass), mType(type), mName(name), mAccessType(access) {}

 MemberBase::MemberBase(const MemberBase& other) : mClass(other.mClass), mType(other.mType), mName(other.mName), mAccessType(other.mAccessType) {}

 MemberBase& MemberBase::operator=(const MemberBase& other) {
   mClass = other.mClass;
   mAccessType = other.mAccessType;
   mType = other.mType;
   mName = other.mName;
   return *this;
 }

 Field::Field(uint64_t offset, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType, const char* strType,
              const char* strName, int nElementCount)
     : MemberBase(pClass, accessType, strType, strName), mType(type), mElementType(mElementType), mOffset(offset), mSize(size), mElementCount(nElementCount) {
   mFieldIdx = pClass->GenFieldIdx();
 }

 StaticField::StaticField(void* pAddr, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType, const char* strType,
                          const char* strName, int nElementCount/* = 1*/)
     : MemberBase(pClass, accessType, strType, strName), mType(type), mElementType(mElementType), mAddr(pAddr), mSize(size), mElementCount(nElementCount) { }

String MethodBase::GetPrefix() const { return Demangle(GetReturnType().name()) + GetClass().GetFullName() + "::" + GetName(); }

 MethodBase::MethodBase(const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs, std::shared_ptr<BaseCallable> cb)
     : MemberBase(pClass, accessType, szType, szName), mArgs(szArgs) {
   mCallable = cb;
   if (cb.get()) {
     mIdentity = GetName();
     mLongIdentity = GetPrefix();
     mLongIdentity += "(";
     const TypeList& argList = cb->GetArgTypeList();
     auto it = argList.begin();
     if (it != argList.end()) {
       mLongIdentity += Demangle((*it)->name());
       it++;
     }
     for (; it != argList.end(); it++) {
       mLongIdentity += "," + Demangle((*it)->name());
     }
     mLongIdentity += ")";
   }
 }

 Method::Method(const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs, bool bVirtual, std::shared_ptr<BaseCallable> cb)
     : MethodBase(pClass, accessType, szType, szName, szArgs, cb), mIsVirtual(bVirtual) {}

 String ConstructorMethod::GetPrefix() const { return GetClass().GetFullName() + "::" + GetName(); }

 Class::Class(const char* strName, const Class* super, std::size_t size, SuperCastFuncPtr superCastFunc, SuperCastConstFuncPtr superCastConstFunc,
     const std::type_info& type, const std::type_info& constType, const std::type_info& ptrType, const std::type_info& constPtrType)
    :  mSuper(super), mName(strName), mSuperCastFunc(superCastFunc), mSuperCastConstFunc(superCastConstFunc), mType(type), mPtrType(ptrType) {

   mFullName = Demangle(type.name());
   if (Utils::StringStartWith(mFullName, "struct ")) {
     mFullName = mFullName.substr(7);
   } else if (Utils::StringStartWith(mFullName, "class ")) {
     mFullName = mFullName.substr(6);
   }

   RegistClassMap(mFullName);
   RegistClassTypeMap(ClassNormalType, type);
   RegistClassTypeMap(ClassConstType, constType);
   RegistClassTypeMap(ClassPointerType, ptrType);
   RegistClassTypeMap(ClassConstPointerType, constPtrType);

   mFieldIdx = 0;
   if (super){
     mFieldIdx = (int)super->mFieldIdx;
   }
 }

 Class::~Class() {
   for (auto it = mFieldMap.begin(); it != mFieldMap.end(); it++) {
     delete it->second;
   }
   mFieldMap.clear();
   mFields.clear();

   for (auto it = mStaticFieldMap.begin(); it != mStaticFieldMap.end(); it++) {
     delete it->second;
   }
   mStaticFieldMap.clear();
   mStaticFieldList.clear();

   for (auto it = mMethodMap.begin(); it != mMethodMap.end(); it++) {
     delete it->second;
   }
   mMethodMap.clear();
   mMethods.clear();

   for (auto it = mStaticMethodMap.begin(); it != mStaticMethodMap.end(); it++) {
     delete it->second;
   }
   mStaticMethodMap.clear();
   mStaticMethods.clear();

   for (auto it = mConstructors.begin(); it != mConstructors.end(); it++) {
     delete *it;
   }
   mConstructors.clear();
 }

 const Class* Class::GetClassByName(const String& strName) { 
   const ClassMap& classMap = GetClassMap();
   auto it = classMap.find(strName);
   if (it == classMap.end()) return nullptr;
   return it->second;
}

const Class* Class::GetClassByType(const std::type_info& type) {
   for (int t = ClassNormalType; t < ClassTypeNum; t++) {
     const TypeInfoMap& typeMap = GetTypeInfoMap((EnumClassType)t);
     auto it = typeMap.find(TypeInfo(type));
     if (it != typeMap.end()) return it->second;
   }
   return nullptr;
}

std::pair<Class::EnumClassType, const Class*> Class::FindClassByType(const std::type_info& type) {
   for (int t = ClassNormalType; t < ClassTypeNum; t++) {
     const TypeInfoMap& typeMap = GetTypeInfoMap((EnumClassType)t);
     auto it = typeMap.find(TypeInfo(type));
     if (it != typeMap.end()) return std::make_pair(static_cast<EnumClassType>(t), it->second);
   }
   return std::make_pair(ClassTypeNum, (const Class*)nullptr);
}

bool Class::IsCastable(const std::type_info& from_cls, const std::type_info& to_cls, void* objptr /*= 0*/) {
   if (from_cls == to_cls) return true;

   if (from_cls == typeid(void*)) {
     return to_cls == typeid(const void*);
   }
   auto fromClass = Class::FindClassByType(from_cls);
   auto toClass = Class::FindClassByType(to_cls);

   if (fromClass.first != Class::ClassTypeNum && fromClass.first == toClass.first) {
     if (toClass.second->IsSameOrSuperOf(*fromClass.second))
       return true;
     else if ((fromClass.first == Class::ClassPointerType || fromClass.first == Class::ClassConstPointerType) && objptr != nullptr) {
       void** objptrptr = static_cast<void**>(objptr);
       return toClass.second->DynamicCastableFromSuper(*objptrptr, fromClass.second);
     }
   }

   if (fromClass.first == Class::ClassNormalType) {
     return toClass.first == Class::ClassConstType && toClass.second->IsSameOrSuperOf(*fromClass.second);
   } else if (fromClass.first == Class::ClassPointerType) {
     if (to_cls == typeid(void*) || to_cls == typeid(const void*)) return true;
     if (toClass.first == Class::ClassConstPointerType) {
       if (toClass.second->IsSameOrSuperOf(*fromClass.second))
         return true;
       else if (objptr != nullptr) {
         void** objptrptr = static_cast<void**>(objptr);
         return toClass.second->DynamicCastableFromSuper(*objptrptr, fromClass.second);
       }
     }
   } else if (fromClass.first == Class::ClassConstPointerType) {
     return (to_cls == typeid(const void*));
   }

   return false;
}

bool Class::ArgsSame(const ArgumentTypeList& argsList1, const ArgumentTypeList& argsList2) {
   if (argsList1.size() != argsList2.size()) {
     return false;
   }
   auto it1 = argsList1.begin();
   auto it2 = argsList2.begin();
   for (; it1 != argsList1.end(); it1++, it2++) {
     if ((*(*it1)) != (*(*it2)) && !IsCastable((*(*it1)), (*(*it2)))) return false;
   }
   return true;
}

const cppfd::Field* Class::GetField(const char* szName, bool bSearchSuper /*= true*/) const { 
   auto it = mFieldMap.find(szName);
   if (it == mFieldMap.end()) {
     if (bSearchSuper && mSuper) return mSuper->GetField(szName, bSearchSuper);
     return nullptr;
   }
   return it->second;
}

const cppfd::StaticField* Class::GetStaticField(const char* szName, bool bSearchSuper /*= true*/) const {
   auto it = mStaticFieldMap.find(szName);
   if (it == mStaticFieldMap.end()) {
     if (bSearchSuper && mSuper) return mSuper->GetStaticField(szName, bSearchSuper);
     return nullptr;
   }
   return it->second;
}

const cppfd::Method* Class::GetMethod(const char* szName, bool bSearchSuper /*= true*/) const {
   auto it = mMethodMap.find(szName);
   if (it == mMethodMap.end()) {
     if (bSearchSuper && mSuper) return mSuper->GetMethod(szName, bSearchSuper);
     return nullptr;
   }
   return it->second;
}

const cppfd::StaticMethod* Class::GetStaticMethod(const char* szName, bool bSearchSuper /*= true*/) const {
   auto it = mStaticMethodMap.find(szName);
   if (it == mStaticMethodMap.end()) {
     if (bSearchSuper && mSuper) return mSuper->GetStaticMethod(szName, bSearchSuper);
     return nullptr;
   }
   return it->second;
}

ClassMap& Class::GetClassMap() { 
   static ClassMap sClassMap;
   return sClassMap;
}

TypeInfoMap& Class::GetTypeInfoMap(EnumClassType eType) { 
    static TypeInfoMap sClassTypeInfoMap[ClassTypeNum]; 
    return sClassTypeInfoMap[eType];
}

bool Class::DynamicCastableFromSuper(void* pSuper, const Class* pSuperClass) const {
    if (this == pSuperClass) return true;
    if (mSuper == nullptr) return false;
    return mSuper->DynamicCastableFromSuper(pSuper, pSuperClass) && DynamicCastFromSuper(pSuper) == pSuper;
}

bool Class::DynamicCastableFromSuper(const void* pSuper, const Class* pSuperClass) const {
    if (this == pSuperClass) return true;
    if (mSuper == nullptr) return false;
    return mSuper->DynamicCastableFromSuper(pSuper, pSuperClass) && DynamicCastFromSuperConst(pSuper) == pSuper;
}

bool Class::IsSuperOf(const Class& cl) const {
    const Class* c = cl.Super();
    while (c) {
     if (c == this) return true;
     c = c->Super();
    }
    return false;
}

void Class::AddField(Field* pField) {
    if (pField == nullptr) return;
    mFieldMap[pField->GetName()] = pField;
    mFields.push_back(pField);
}

void Class::AddStaticField(StaticField* pField) {
    if (pField == nullptr) return;
    mStaticFieldMap[pField->GetName()] = pField;
    mStaticFieldList.push_back(pField);
}

void Class::AddMethod(Method* pMethod) {
    if (pMethod == nullptr) return;
    mMethodMap[pMethod->GetName()] = pMethod;
    mMethods.push_back(pMethod);
}

void Class::AddStaticMethod(StaticMethod* pMethod) {
    if (pMethod == nullptr) return;
    mStaticMethodMap[pMethod->GetName()] = pMethod;
    mStaticMethods.push_back(pMethod);
}

void Class::AddConstructorMethod(ConstructorMethod* pMethod) {
    if (pMethod == nullptr) return;
    mConstructors.push_back(pMethod);
}

FieldRegister::FieldRegister(uint64_t offset, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType, const char* szType,
                             const char* szName, int nCount) {
    Field* pField = new Field(offset, size, type, elementType, pClass, accessType, szType, szName, nCount);
    (const_cast<Class*>(pClass))->AddField(pField);
 }

 StaticFieldRegister::StaticFieldRegister(void* pAddr, std::size_t size, const std::type_info& type, const std::type_info& elementType, const Class* pClass, EnumAccessType accessType,
                                          const char* szType, const char* szName, int nCount) {
    StaticField* pField = new StaticField(pAddr, size, type, elementType, pClass, accessType, szType, szName, nCount);
    (const_cast<Class*>(pClass))->AddStaticField(pField);
 }

 MethodRegister::MethodRegister(std::shared_ptr<BaseCallable> pCb, const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs, bool bVirtual) {
    Method* pMethod = new Method(pClass, accessType, szType, szName, szArgs, bVirtual, pCb);
    (const_cast<Class*>(pClass))->AddMethod(pMethod);
 }

 StaticMethodRegister::StaticMethodRegister(std::shared_ptr<BaseCallable> pCb, const Class* pClass, EnumAccessType accessType, const char* szType, const char* szName, const char* szArgs) {
    StaticMethod* pMethod = new StaticMethod(pClass, accessType, szType, szName, szArgs, pCb);
    (const_cast<Class*>(pClass))->AddStaticMethod(pMethod);
 }

 ConstructorMethodRegistor::ConstructorMethodRegistor(std::shared_ptr<BaseCallable> pCb, std::shared_ptr<BaseCallable> pPlacementCb, const Class* pClass, EnumAccessType accessType, const char* szType,
                                                      const char* szName, const char* szArgs) {
    ConstructorMethod* pMethod = new ConstructorMethod(pClass, accessType, szType, szName, szArgs, pCb, pPlacementCb);
    (const_cast<Class*>(pClass))->AddConstructorMethod(pMethod);
 }

 }  // namespace cppfd