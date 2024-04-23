#pragma  once
#include "Reflection.h"
#include "RefJson.h"

struct TestStruct {
  REF_EXPORT int mVal;
  REF_EXPORT cppfd::JsonArray<cppfd::String> StrArray;
  REF_EXPORT cppfd::JsonMap<int> IntValMap;

  TestStruct(){ 
	  mVal = 0;
  }
};

struct TestBaseObj 
{
  REF_EXPORT char Content[20];
  REF_EXPORT bool BoolVal;
  REF_EXPORT int16_t ShortVal;
  REF_EXPORT int32_t IntVal;
  REF_EXPORT int64_t Int64Val;
  REF_EXPORT cppfd::String StrVal;
  REF_EXPORT float FloatVal;
  REF_EXPORT double DoubleVal;
  REF_EXPORT TestStruct SubObj;
  REF_EXPORT cppfd::JsonArray<TestStruct> SubObjArray;
  REF_EXPORT cppfd::JsonMap<TestStruct> SubObjMap;

  TestBaseObj() { 
	Content[0] = 0;
    DoubleVal = 123.456789123456;
    FloatVal = 123.5f;
    Int64Val = 123456789101112;
    StrVal = "TestVal";
    ShortVal = 8192;
    BoolVal = true;
    IntVal = 100;
  }
};