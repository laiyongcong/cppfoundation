# 简介
cppfoundation是一个c++基础库，这部分代码曾经在某游戏项目中使用过，经过一段时间思考，觉得曾经写过的一些代码有一定的价值，于是把这些部分重新整理，供大家参考。 本库主要目的是完成跨平台的封装，以编程框架的形式降低在C++下进行编程的门槛。

该基础库主要包含：
* c++反射（已完成）
* 基于c++反射的对象的json序列化和反序列化（已完成）
* 基于c++反射的csv文件读取（计划实现，也可以实现yaml文件读取，依赖于yaml-cpp）
* 线程框架（计划中）
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
const Class* pClass = Class::GetClassByType(typeid(RefExample));
const Field* pField = pClass->GetField("IntVal");
const Method* pMethod = pClass->GetMethod(TestMethod);

RefExample example;
pField->Set(&example, 100);
int nValue = 0;
pField->Get(&example, nValue);
pMethod->Invoke(10, 20);
```
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
* 基础类型的JsonArray和JsonMap反射代码已经包含在库里面，不需要自行手写；参与反射的其他类型的JsonArray和JsonMap反射代码也会被工具自动自动生成

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