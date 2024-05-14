#include "Prerequisites.h"
#include "TestHeader.h"
#include "TestHeader2.h"
#include "RefJson.h"
#include "Thread.h"
#include "Log.h"
#include "TabFile.h"

using namespace cppfd;

std::atomic_int gCounter(0);

int ServerMsg::Ping(Connecter* pConn, const char* szBuff, uint32_t uBuffLen) {
  LOG_TRACE("Recv Ping from %s msg:%s", pConn->Info().c_str(), szBuff);
  static String strMsg = "Hello Client!!!!!!!";
  pConn->Send("Pong", strMsg.c_str(), (uint32_t)strMsg.size());
  return 0;
}

int ClientMsg::Pong(Connecter* pConn, const char* szBuff, uint32_t uBuffLen) {
  LOG_TRACE("Recv Pong from %s msg:%s", pConn->Info().c_str(), szBuff);
  static String strMsg = "Hello Server!!!!!!!";
  pConn->Send("Ping", strMsg.c_str(), (uint32_t)strMsg.size());
  gCounter++;
  return 0;
}

class TestClient : public TcpEngine {
 public:
  TestClient(uint32_t uNetThreadNum, BaseNetDecoder* pDecoder, const std::type_info& tMsgClass) 
  : TcpEngine(uNetThreadNum, pDecoder, tMsgClass, -1){}

  void OnConnecterCreate(Connecter* pConn) override { 
    TcpEngine::OnConnecterCreate(pConn);
    String strMsg = "Hello Server!!!!!!!";
    pConn->Send("Ping", strMsg.c_str(), (uint32_t)strMsg.size()); 
  }
};

void NetTest() {
  SockInitor initor;
  LogConfig cfg;
  cfg.ProcessName = "testLog";
  cfg.LogLevel = ELogLevel_Warning;
  Log::Init(cfg);
  TcpEngine testServer(2, (BaseNetDecoder*)&g_NetHeaderDecoder, typeid(ServerMsg), 9100);
  TestClient testClient(2, (BaseNetDecoder*)&g_NetHeaderDecoder, typeid(ClientMsg));
  testServer.SetCrypto(DefaultNetCryptoFunc, DefaultNetCryptoFunc, 12345, 12345);
  testClient.SetCrypto(DefaultNetCryptoFunc, DefaultNetCryptoFunc, 12345, 12345);

  testServer.Start();
  testClient.Start();

  int nTimeout = 10000000;
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);
  testClient.Connect("127.0.0.1", 9100, &nTimeout);

  int nCounter = 0;
  while (nCounter < 100)
  {
    Thread::Milisleep(1000);
    nCounter++;
  }
  nCounter = gCounter;
  LOG_WARNING("total:%d", nCounter);
  Log::Destroy();
}


void ThreadTest() { 
  Thread p1, p2, p3,c1,c2,c3;
  ThreadPool pool(30);
  p1.Start("p1");
  p2.Start("p2");
  p3.Start("p3");
  c1.Start("c1");
  c2.Start("c2");
  c3.Start("c3");
  pool.Start("pool");
  pool.BroadCast([]() { std::cout << "hello world! im " << Thread::GetCurrentThread()->GetName() << std::endl; });
  std::atomic_int gVal(0);

  p1.Post([&]() {
    int i = 0;
    while (i < 10000000) {
      /*c1.Post([&]() { gVal++; });
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });*/
      // Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  p2.Post([&]() {
    int i = 0;
    while (i < 10000000) {
      /*c1.Post([&]() { gVal++; });
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });*/
      // Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  p3.Post([&]() {
    int i = 0;
    while (i < 10000000) {
      /*c1.Post([&]() { gVal++; });
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });*/
      // Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  c1.Post([&]() {
    int i = 0;
    while (i < 10000000) {
      /*c1.Post([&]() { gVal++; });
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });*/
      // Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  c2.Post([&]() {
    int i = 0;
    while (i < 10000000) {
      /*c1.Post([&]() { gVal++; });
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });*/
      // Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  c3.Post([&]() {
    int i = 0;
    while (i < 10000000) {
      /*c1.Post([&]() { gVal++; });
      c2.Post([&]() { gVal++; });
      c3.Post([&]() { gVal++; });*/
      // Thread::Milisleep(1);
      pool.Post([&]() { gVal++; });
      i++;
    }
  });

  while (gVal < 60000000) {
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

void TestLog() { 
  LogConfig cfg;
  cfg.ProcessName = "testLog";
  Log::Init(cfg);
  uint64_t uTime1 = cppfd::Utils::GetCurrentTime();
  for (int i = 0; i < 100000; i++) {
    LOG_TRACE("this is test log %d", i);
  }
  uint64_t uTime2 = cppfd::Utils::GetCurrentTime();
  std::cout << "log cost " << uTime2 - uTime1 << std::endl;
  Log::Destroy();
}

int main(int argc, char** argv) {

    NetTest();
  /*TestLog();

  uint64_t uTime1 = cppfd::Utils::GetTimeMiliSec();
  ThreadTest();
  uint64_t uTime2 = cppfd::Utils::GetTimeMiliSec();
  std::cout << "cost " << uTime2 - uTime1 << std::endl;*/

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

  TestClass* pParentClassPtr = pTestSubClass->NewInstance<TestClass>();
  if (pParentClassPtr) {
    pParentClassPtr->VirtualFunc();
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
  testStruct1.StrArray.mItems.push_back("{bbb,\"\r\n\tccc}[]");
  testStruct1.IntValMap.mObjMap["val1"] = 1;
  testStruct1.IntValMap.mObjMap["val2"] = 2;

  TestStruct testStruct2;
  testStruct2.mVal = 2;
  testStruct2.StrArray.mItems.push_back("b");
  testStruct2.StrArray.mItems.push_back("{bbb\r\n\tcc,\"c}[]");
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


  baseObj.Content[0] = '"';
  baseObj.Content[1] = 0;
  baseObj2.Content[0] = 0;

  TabFile<TestBaseObj, ','> tabFile, tabFile2;
  tabFile.mItems.push_back(baseObj);
  tabFile.mItems.push_back(baseObj2);

  
  tabFile.Save("./test.csv");
  
  tabFile2.OpenFromTxt("./test.csv");

  tabFile2.Save("./test2.csv");
  
  return 0;
}