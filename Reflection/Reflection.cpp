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
                          const char* strName, int nElementCount)
     : MemberBase(pClass, accessType, strType, strName), mType(type), mElementType(mElementType), mAddr(pAddr), mSize(size), mElementCount(nElementCount) { }

 Class::Class(const char* strName, const Class* super, std::size_t size, NewInstanceFuncPtr newFunc, AllocaInstanceFuncPtr allocaFunc, SuperCastFuncPtr superCastFunc,
             SuperCastConstFuncPtr superCastConstFunc, const std::type_info& type, const std::type_info& constType, const std::type_info& ptrType, const std::type_info& constPtrType)
    :  mSuper(super), mName(strName), mNewFunc(newFunc), mAllocaFunc(allocaFunc), mSuperCastFunc(superCastFunc), mSuperCastConstFunc(superCastConstFunc), mType(type), mPtrType(ptrType) {

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

}  // namespace cppfd