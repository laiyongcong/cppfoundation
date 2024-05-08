# 简介
cppfoundation是一个c++基础库，这部分代码曾经在某游戏项目中使用过，经过一段时间思考，觉得曾经写过的一些代码有一定的价值，于是把这些部分重新整理，供大家参考。 

该基础库主要包含：
* c++反射（已完成）
* 基于c++反射的对象的json序列化和反序列化（已完成）
* 基于c++反射的csv文件读取（已完成，也可以实现yaml文件读取，依赖于yaml-cpp）
* 线程框架（已完成）
* 日志系统（已完成）
* 网络通信框架（计划中）

# C++反射（reflection）
## 反射工具
相对于虚幻的UHT，本基础库反射工具基于llvm项目，通过开发的clang工具来分析项目头文件中的反射标记，生成反射代码，对C++的语法支持得更彻底。工具源码详见Tools/ReflectionTool, 目前Bin目录下是已经编译好的windows平台下的程序(其它平台可以自行按照CMakeList中的说明自行编译)，推荐在visual studio工程中使用PreBuild方式触发反射代码生成。

在Reflection.h中，反射标记如下
```
#ifndef REF_EXPORT_FLAG
#  define REF_EXPORT
#else
#  define REF_EXPORT struct CPPFD_JOIN(__refflag_, __LINE__) {};
#endif
```
"__refflag_"将只在反射工具看到的代码中存在，而原工程中REF_EXPORT是一个空的宏。被标记过的成员变量或者函数，将被生成反射代码，如下：
```
class RefExample {
  public:  
    REF_EXPORT int IntVal;
    REF_EXPORT void TestMethod(int a, int b);
};
```
在工程配置了适当时机生成反射代码的前提下，以下代码由反射工具自动生成
```
static const ReflectiveClass<RefExample> g_refRefExample = ReflectiveClass<RefExample>("RefExample")
	.RefField(&RefExample::IntVal,"IntVal")
    .RefMethod(&RefExample::TestMethod, "TestMethod");
```
在业务代码中，可以通过反射代码，访问对象的成员变量，或者调用其方法
```
const Class* pClass = Class::GetClass(typeid(RefExample));
const Field* pField = pClass->GetField("IntVal");
const Method* pMethod = pClass->GetMethod(TestMethod);

RefExample example;
pField->Set(&example, 100);
int nValue = 0;
pField->Get(&example, nValue);
pMethod->Invoke(10, 20);
```
关于容器部分的支持，见后面介绍的JsonArray和JsonMap。

## 使用CMake结合反射工具，在适当时机生成反射代码
```
#收集工程所有目录
get_target_property(include_dirs ${PROJECT_NAME} INCLUDE_DIRECTORIES)
#收集工程所有的依赖目录（转换为绝对路径），写入到文件${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.dirs
GenReflectionDirListFromSourceDirs(${PROJECT_NAME} ${include_dirs})  
#收集工程所有的头文件，写入到文件 ${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.reflection
GenReflectionHeaderListFromSourceFiles(${PROJECT_NAME} ${${PROJECT_NAME}_All_Sources}) 

if(WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_SOURCE_DIR}/Bin/ReflectionTool
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.reflection"
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/Reflection.generated.cpp"  #生成的反射代码
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.dirs"
		"cppfd"   #后面是相关的namespace
	)
else() #linux下似乎不能用prebuild
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/Bin/ReflectionTool
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.reflection"
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/Reflection.generated.cpp" 
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.dirs"
		"cppfd"
		)
endif()
```
## 关于反射系统的注意事项
* 被反射的类或者结构体目前不支持多重继承，只允许有一个父类被反射。在多重继承的情况下，只要不是所有的父类都参与反射，是允许的
* 基础类型的JsonArray和JsonMap反射代码已经包含在库里面，不需要自行手写；参与反射的其他类型的JsonArray和JsonMap反射代码也会被工具自动生成
* Class实例创建时，会向全局的静态容器里插入数据，因此运行时的反射需要注意线程安全，建议全部反射代码由工具自动生成，在程序启动时完成所有的反射

# Json解析
Utils/RefJson 实现了基于c++反射的json读写，即类对象实例或结构体实例，可以进行json的序列化和反序列化，json解析部分使用了[fasterjson](https://github.com/calvinwilliams/fasterjson), 形如以下的结构体：
```
struct TestObj {
    REF_EXPORT int x;
    REF_EXPORT int y;
    REF_EXPORT int z;
};
struct TestStruct {
    REF_EXPORT int a;
    REF_EXPORT std::string msg;
    REF_EXPORT JsonArray<int> arr;
    REF_EXPORT TestObj obj;
};
```
序列化后的json可能如下：
```
{"a":1, "msg":"hello world", "arr":[1,2,3], "obj":{"x":4, "y":5, "z":6}}
```
测试代码如下：
```
std::string strJson = "{\"a\":1, \"msg\":\"hello world\", \"arr\":[1,2,3], \"obj\":{\"x\":4, \"y\":5, \"z\":6}}";
TestStruct testS;
JsonBase::FromJsonString(&testS, strJson); //读取json的数据，反序列化到结构体中
testS.a = 100;
std::cout << JsonBase::ToJsonString(&testS) << std::endl;
```
结构体之间支持嵌套， 但是被嵌套进去的类或者结构体必须是反射过的

## Json解析支持的基本类型
```
bool JsonBase::IsBuildInType(const std::type_info& tinfo) {
  if (tinfo == typeid(int8_t) || 
      tinfo == typeid(uint8_t) || 
      tinfo == typeid(int16_t) || 
      tinfo == typeid(uint16_t) || 
      tinfo == typeid(int32_t) || 
      tinfo == typeid(uint32_t) ||
      tinfo == typeid(int64_t) || 
      tinfo == typeid(uint64_t) || 
      tinfo == typeid(float) || 
      tinfo == typeid(double) || 
      tinfo == typeid(bool) || 
      tinfo == typeid(String)
  )
      return true;
  return false;
}
```

## JsonArray和JsonMap
* JsonArray可以包装基本类型或者反射过的类型，其表现形式为json中的变长数组
* JsonMap 可以映射一个json中数值类型相同的key-value对，这些key-value对被包含在“{}”中，主要为了解决key不确定的情形下遇到的问题。其中value必须是基本类型，或者被反射过的类型。

# 线程
## 无锁线程安全队列（见Utils/Queue.h）
* T1V1Queue 提供单个生产者和单个消费者之间的一对一线程安全队列
* TMultiV1Queue 提供多个生产者对单个消费者之间的多对一线程安全队列
* TQueue 多生产者和多消费者完全线程安全的队列
 
线程之间通过无锁队列进行高效的信息交互，本框架建议所有的线程都采用封装好的cppfd::Thread来实现，cppfd::Thread的设计出发点是：把线程看做无差别的工作者（cppfd::ThreadPool看作工作者组），任务被封装成std::function以无锁队列的方式提交给这些工作者（组）完成，任务交互提供以下几种方式：
* Post 向线程或者线程池异步提交任务；
* Dispatch 与Post相同，但是会检查是否当前线程给自己提交，若当前线程自己给自己提交则立即执行；
* Async 向目标线程（池）异步提交任务，目标线程完成任务后，当前线程（必须是cppfd::Thread）执行回调
* Invoke 向目标线程（池）提交任务，并等待任务的结果。
* 线程池BroadCast， 向线程池所有的线程发送一个异步任务。

多线程编程中，可能会遇到多个任务分解问题，大的任务划分为子任务后，由线程池完成各个子任务，结果最终整合到一起继续进行后续处理，这种情况可以使用cppfd::StepsTask

在多线程环境中，线程之间相互调用，线程回调可能会遇到发起任务的线程已经退出的情形，此时回调会导致程序崩溃，因此提供了cppfd::ThreadKeeper来判断或者防止回调时线程已经结束，避免程序的崩溃。

# 日志系统
见Log/Log.h 日志的Init会创建一个独立的日志线程，日志线程每100毫秒会把缓冲区刷到日志文件和标准输出，日志等级分为FATAL、ERROR、WARNING、INFO、DEBUG，其中ERROR以及FATAL除了在INFO日志文件中作为一个整体打印，也会单独输出到ERROR日志文件中。

日志系统可以设置切分方式：天、小时、分钟，为了避免单个日志文件过大，日志文件达到单个文件大小（MB）限制后会自动重命名。

日志系统可以设置日志的保留时间，单位为天。

# 类csv文件的读写
具体实现见Utils/TabFile.h， csv文件的读写依赖于反射，文件的首行是被反射对象的字段列表，文件中可以增加注释行(需要以“#”开头),TabFile中的Delim一般为“\t”或者“,”，T需要是被反射过的类型，其主要结构如下：
```
template <typename T, const char Delim>
class TabFile {
 public:
  std::vector<T> mItems;
 public:
  bool OpenFromTxt(const char* szTabFile, int nIgnoreLines = 0) { ... }
  bool Save(const char* szTabFile) const { ... }
  ...
};
```
