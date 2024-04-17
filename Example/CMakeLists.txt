cmake_minimum_required(VERSION 3.10.2)

project(Example)

include(Example.cmake)

include_directories(${CMAKE_INSTALL_PREFIX}/include/)
include_directories(${${PROJECT_NAME}_Local_Include_Dirs})

FILE(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Generated/Reflection.generated.cpp "")

FILE(GLOB Generated_Sources "Generated/*.cpp")
source_group(Generated\ Files
	FILES
	${Generated_Sources}
)

list(APPEND ${PROJECT_NAME}_All_Sources ${Generated_Sources})

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_All_Sources})

if(UNIX)
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES  LINK_FLAGS "-static-libstdc++")
	target_link_libraries(${PROJECT_NAME} "-lrt")
	target_link_libraries(${PROJECT_NAME} "-Wl,--start-group")
endif()

if(WIN32)
	target_link_libraries(${PROJECT_NAME} 
		optimized ${CMAKE_INSTALL_PREFIX}/lib/cppfoundation.lib
		debug ${CMAKE_INSTALL_PREFIX}/lib/cppfoundationd.lib)
else()
	target_link_libraries(${PROJECT_NAME} ${CMAKE_INSTALL_PREFIX}/lib/cppfoundation.a)
endif()

if(UNIX)
	target_link_libraries(${PROJECT_NAME} "-Wl,--end-group")
endif()

get_target_property(include_dirs ${PROJECT_NAME} INCLUDE_DIRECTORIES)
GenReflectionDirListFromSourceDirs(${PROJECT_NAME} ${include_dirs})
GenReflectionHeaderListFromSourceFiles(${PROJECT_NAME} ${${PROJECT_NAME}_All_Sources})

if(WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_SOURCE_DIR}/Bin/ReflectionTool
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.reflection"
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/Reflection.generated.cpp" 
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.dirs"
		"cppfd"
	)
else() #linux下似乎不能用prebuild
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/Bin/ReflectionTool
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.reflection"
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/Reflection.generated.cpp" 
		"${CMAKE_CURRENT_SOURCE_DIR}/Generated/${PROJECT_NAME}.dirs"
		"cppfd"
		)
endif()
