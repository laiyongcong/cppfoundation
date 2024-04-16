#pragma  once
#include "Reflection.h"

class TestClass {
 public:
  REF_EXPORT int mIntVal;
  REF_EXPORT std::string mStrVal;
  REF_EXPORT static int mStaticIntVal;
  REF_EXPORT TestClass() : mIntVal(0), mStrVal() {}
  REF_EXPORT TestClass(int nVal, const std::string& strVal) : mIntVal(nVal), mStrVal(strVal) {}

  REF_EXPORT void Output() const;
  REF_EXPORT static void StaticOutput();
  REF_EXPORT void SetIntVal(int nVal) { mIntVal = nVal; }
  REF_EXPORT void SetStringVal(const std::string& strVal) { mStrVal = strVal; }

  REF_EXPORT virtual void VirtualFunc() const;
};

class TestSubClass : public TestClass {
 public:
  REF_EXPORT TestSubClass() { ::memset(mArray, 0, sizeof(mArray)); }
  REF_EXPORT int mArray[10];
  REF_EXPORT virtual void VirtualFunc() const override { std::cout << "virturl func in TestSubClass" << std::endl; }
};