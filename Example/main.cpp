#include "Prerequisites.h"
#include "TestHeader.h"

void main(int argc, char** argv) {
  using namespace cppfd;
  const Class* pTestClass = Class::GetClassByType(typeid(TestClass));
  const Class* pTestSubClass = Class::GetClassByType(typeid(TestSubClass));
  if (pTestClass == nullptr || pTestSubClass == nullptr) {
    std::cout << "refection failed" << std::endl;
  }

  TestSubClass subobj;
  TestClass obj;
  const Method* pVirtualMethod = pTestSubClass->GetMethod("VirtualFunc");
  if (pVirtualMethod)
  {
    pVirtualMethod->Invoke(&subobj);
    pVirtualMethod->Invoke(&obj);
  }

  TestSubClass* pConstructorTest = pTestSubClass->NewInstance<TestSubClass>();
  TestClass* pCon2 = pTestClass->NewInstance<TestClass, int, const std::string&>(1, "aaa");

  char szTmpBuff[2048] = {0};
  TestClass* pAllocaTest = pTestClass->Alloca<TestClass, int, const std::string&>(szTmpBuff, 2, "bbb");
  pAllocaTest = pTestClass->Alloca<TestClass>(szTmpBuff);

  if (Class::IsCastable(typeid(TestClass*), typeid(TestSubClass*), &subobj))
  {
    std::cout << "err cast" << std::endl;
  }

}