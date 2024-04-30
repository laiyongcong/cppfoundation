#include "Prerequisites.h"
#include "TestHeader.h"
#include "TestHeader2.h"
#include "RefJson.h"
#include "Thread.h"

using namespace cppfd;



void ThreadTest() { 
  Thread p1, p2, p3,c1,c2,c3;
  ThreadPool pool(3);
  p1.Start("p1");
  p2.Start("p2");
  p3.Start("p3");
  c1.Start("c1");
  c2.Start("c2");
  c3.Start("c3");
  pool.Start("pool");
  pool.BroadCast([]() { std::cout << "hello world! im " << Thread::GetCurrentThread()->GetName() << std::endl; });
  std::atomic_int gVal = 0;

  p1.Post([&]() {
    int i = 0;
    while ( i < 1000){
      c1.Post([&](){ gVal++;});
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });
      //Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  p2.Post([&]() {
    int i = 0;
    while (i < 1000) {
      c1.Post([&]() { gVal++; });
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });
      //Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  p3.Post([&]() {
    int i = 0;
    while (i < 1000) {
      c1.Post([&]() { gVal++; });
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });
      //Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  while (gVal < 12000) {
    Thread::Milisleep(1);
  }
  p1.Stop();
  p2.Stop();
  p3.Stop();
  c1.Stop();
  c2.Stop();
  c3.Stop();
  pool.Stop();
  std::cout << "thread test ok!" << std::endl;
}

void main(int argc, char** argv) {
  ThreadTest();

  const Class* pTestClass = Class::GetClass<TestClass>();
  const Class* pTestSubClass = Class::GetClass<TestSubClass>();
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
    pField->Get(&subobj, nVal);
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
        pField->GetByIdx(&subobj, nval, i);
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

  auto pStaticMethod = pTestClass->GetStaticMethod("StaticOutput");
  if (pStaticMethod)
  {
      pStaticMethod->Invoke();
  }

  pStaticMethod = pTestClass->GetStaticMethod("StaticOutput2");
  if (pStaticMethod) {
      pStaticMethod->Invoke<int>();
  }
  pStaticMethod = pTestClass->GetStaticMethod("StaticOutput3");
  if (pStaticMethod) {
      pStaticMethod->Invoke<int, int>(3);
  }

  pStaticMethod = pTestClass->GetStaticMethod("StaticOutput4");
  if (pStaticMethod) {
      pStaticMethod->Invoke(4);
  }
  subobj.SetStringVal("{\"a\":1}");
  String strJson = JsonBase::ToJsonString(&subobj);
  std::cout << strJson << std::endl;
  TestSubClass subobj2;
  JsonBase::FromJsonString(&subobj2, strJson);

  TestStruct testStruct1;
  testStruct1.mVal = 1;
  testStruct1.StrArray.mItems.push_back("a");
  testStruct1.StrArray.mItems.push_back("{bbb\r\n\tccc}[]");
  testStruct1.IntValMap.mObjMap["val1"] = 1;
  testStruct1.IntValMap.mObjMap["val2"] = 2;

  TestStruct testStruct2;
  testStruct2.mVal = 2;
  testStruct2.StrArray.mItems.push_back("b");
  testStruct2.StrArray.mItems.push_back("{bbb\r\n\tccc}[]");
  testStruct2.IntValMap.mObjMap["val1"] = 1;
  testStruct2.IntValMap.mObjMap["val2"] = 2;

  TestBaseObj baseObj, baseObj2;
  baseObj.SubObjArray.mItems.push_back(testStruct1);
  baseObj.SubObjArray.mItems.push_back(testStruct2);

  baseObj.SubObjMap.mObjMap["testStruct1"] = testStruct1;
  baseObj.SubObjMap.mObjMap["testStruct2"] = testStruct2;

  strJson = JsonBase::ToJsonString(&baseObj);
  std::cout << strJson << std::endl;

  JsonBase::FromJsonString(&baseObj2, strJson);

  JsonArray<int> TestArray, TestArray2;
  TestArray.mItems.push_back(10);
  TestArray.mItems.push_back(1);
  TestArray.mItems.push_back(100);

  strJson = JsonBase::ToJsonString(&TestArray);
  std::cout << strJson << std::endl;

  JsonBase::FromJsonString(&TestArray2, strJson);

  JsonArray<TestStruct> testObjArray, testObjArray2;
  testObjArray.mItems.push_back(testStruct1);
  testObjArray.mItems.push_back(testStruct2);

  strJson = JsonBase::ToJsonString(&testObjArray);
  std::cout << strJson << std::endl;

  JsonBase::FromJsonString(&testObjArray2, strJson);


  JsonMap<int> testMap, testMap2;
  testMap.mObjMap["a"] = 1;
  testMap.mObjMap["b"] = 2;

  strJson = JsonBase::ToJsonString(&testMap);
  std::cout << strJson << std::endl;

  JsonBase::FromJsonString(&testMap2, strJson);

  JsonMap<TestStruct> testObjMap, testObjMap2;
  testObjMap.mObjMap["a"] = testStruct1;
  testObjMap.mObjMap["b"] = testStruct2;

  strJson = JsonBase::ToJsonString(&testObjMap);
  std::cout << strJson << std::endl;

  JsonBase::FromJsonString(&testObjMap2, strJson);
}