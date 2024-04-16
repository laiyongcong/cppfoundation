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
}