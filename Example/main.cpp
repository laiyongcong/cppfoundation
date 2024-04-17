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
  if (pConstructorTest)
  {
    delete pConstructorTest;
    pConstructorTest = nullptr;
    std::cout << "new instance test ok" << std::endl;
  }
  
  TestClass* pCon2 = pTestClass->NewInstance<TestClass, int, const std::string&>(1, "aaa");
  if (pCon2)
  {
    std::cout << "new instance with params test ok" << std::endl;
    delete pCon2;
    pCon2 = nullptr;
  }
  

  char szTmpBuff[2048] = {0};
  TestClass* pAllocaTest = pTestClass->Alloca<TestClass, int, const std::string&>(szTmpBuff, 2, "bbb");
  if (pAllocaTest)
  {
    std::cout << "allocate with params test ok" << std::endl;
  }
  pAllocaTest = pTestClass->Alloca<TestClass>(szTmpBuff);
  if (pAllocaTest)
  {
    std::cout << "allocate test ok" << std::endl;
  }

  auto pField = pTestSubClass->GetField("mIntVal");
  if (pField)
  {
    int nVal;
    pField->Set(&subobj, 5);
    pField->Get(nVal, &subobj);
    std::cout << "Get field val:" << nVal << " result:" << (nVal == 5) << std::endl;
  }

  pField = pTestSubClass->GetField("mArray");
  if (pField)
  {
    std::cout << "test array field:";
      for (int i = 0; i < pField->GetElementCount(); i++)
      {
        pField->SetByIdx(&subobj, i, i);
        int nval;
        pField->GetByIdx(nval, &subobj, i);
        std::cout << nval << " ";
      }
      std::cout << std::endl;
  }

  auto pStaticField = pTestSubClass->GetStaticField("mStaticIntVal");
  if (pStaticField)
  {
      pStaticField->Set(100);
      int nval;
      pStaticField->Get(nval);
      std::cout << "Get static field val:" << nval << " result:" << (nval == 100) << std::endl;
  }

}