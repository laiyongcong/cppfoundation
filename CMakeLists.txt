cmake_minimum_required(VERSION 3.10.2)

project(cppfoundation)

include(cppfoundation.cmake)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH}")

include(CommonHeader)
include(BasicFunc)

if(WIN32)
SET(CMAKE_DEBUG_POSTFIX  "d")
add_definitions(/W4)
endif()

if(MSVC AND MSVC_VERSION GREATER 1800 AND NOT(ANDROID))
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd\"4311\" /wd\"4312\" /wd\"4477\" /wd\"4302\"")
endif()

if (APPLE OR ANDROID OR UNIX)
	if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
		CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	endif()
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
	set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fPIC")
endif()

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_All_Sources})
include_directories(${${PROJECT_NAME}_Local_Include_Dirs})

GenHeaderFileList(${${PROJECT_NAME}_All_Sources})  #gen ${${PROJECT_NAME}_Header_Files}  

install(FILES 
	${${PROJECT_NAME}_Header_Files}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/include/
)

install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)