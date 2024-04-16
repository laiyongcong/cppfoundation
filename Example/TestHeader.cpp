#include "TestHeader.h"

int TestClass::mStaticIntVal = 0;

void TestClass::Output() const { std::cout << "val:" << mIntVal << " str:" << mStrVal << std::endl; }

void TestClass::StaticOutput() { std::cout << "hello world" << std::endl; }

void TestClass::VirtualFunc() const { std::cout << "virtual func in TestClass" << std::endl; }
