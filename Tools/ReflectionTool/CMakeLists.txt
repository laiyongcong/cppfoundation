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