cmake_minimum_required(VERSION 3.10.2)

project(Example)

include(Example.cmake)

foreach(_dir ${cppfoundation_Local_Include_Dirs})
include_directories("../${_dir}")
endforeach()

include_directories(${${PROJECT_NAME}_Local_Include_Dirs})

FILE(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Generated/Reflection.generated.cpp "")

FILE(GLOB Generated_Sources "Generated/*.cpp")
source_group(Generated\ Files
	FILES
	${Generated_Sources}
)

list(APPEND ${PROJECT_NAME}_All_Sources ${Generated_Sources} ${cppfoundation_All_Sources})

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_All_Sources})

if(UNIX)
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES  LINK_FLAGS "-static-libstdc++")
	target_link_libraries(${PROJECT_NAME} "-lrt")
	target_link_libraries(${PROJECT_NAME} "-Wl,--start-group")
endif()

if(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread)
else()
endif(UNIX)

if(UNIX)
	target_link_libraries(${PROJECT_NAME} "-Wl,--end-group")
endif()

#收集工程所有目录
get_target_property(include_dirs ${PROJECT_NAME} INCLUDE_DIRECTORIES)
#收集工程所有的依赖目录（转换为绝对路径），写入到文件${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.dirs
GenReflectionDirListFromSourceDirs(${PROJECT_NAME} ${include_dirs})  
#收集工程所有的头文件，写入到文件 ${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.reflection
GenReflectionHeaderListFromSourceFiles(${PROJECT_NAME} ${${PROJECT_NAME}_All_Sources}) #

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
