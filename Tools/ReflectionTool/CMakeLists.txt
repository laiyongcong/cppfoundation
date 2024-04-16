set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

add_clang_tool(ReflectionTool
  ReflectionTool.cpp
  )

clang_target_link_libraries(ReflectionTool
	PRIVATE
	clangAST
	clangBasic
	clangDriver
	clangFrontend
	clangRewriteFrontend
	clangTooling
  )
  
# 使用https://github.com/llvm/llvm-project， 推荐版本17.0.6，需要把目录拷贝到clang/tools 目录下， 修改CMakeList.txt， 新增子目录：add_clang_subdirectory(ReflectionTool)
# 在llvm目录下，创建build目录，然后：cmake -DLLVM_EXTERNAL_CLANG_SOURCE_DIR=/root/src/clang -DLLVM_TOOL_CLANG_BUILD=True -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ../