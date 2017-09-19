# For 64-bit build---> cmake -G "Visual Studio 15 2017 Win64" ../
# to open solution--> start <project name>.sln
cmake_minimum_required(VERSION 3.5.1)
project (Fbx_Parser)

# set release or debug builds
if(CMAKE_CONFIGURATION_TYPES)
   set(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo Debug)
   set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
     "Reset the configurations to what we need"
     FORCE)
 endif()
set(CMAKE_CXX_STANDARD 11)


# set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# set flags for debug and release mode
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-g HAS_DEBUG)
check_cxx_compiler_flag(-Wall HAS_WALL)
check_cxx_compiler_flag(-O0 HAS_OPT0)
check_cxx_compiler_flag(-O3 HAS_OPT3)
check_cxx_compiler_flag(-fmessage-length=0 HAS_MSG)
check_cxx_compiler_flag(-std=c++11 HAS_CNEW)
check_cxx_compiler_flag(-ldl HAS_LDL)
if(HAS_DEBUG)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
endif()
if(HAS_WALL)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif()
if(HAS_OPT0)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
endif()
if(HAS_OPT3)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()
if(HAS_MSG)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fmessage-length=0")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fmessage-length=0")
endif()
if(HAS_CNEW)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11")
endif()
if(UNIX)
    if(NOT HAS_LDL)
        message(FATAL_ERROR "-ldl not found in system")
    endif()
endif()
# include directories for project
find_path(FBX_PATH fbxsdk.h
	PATHS
	${CMAKE_SOURCE_DIR}/FBX_SDK/include)
if(NOT FBX_PATH)
	message(FATAL_ERROR "FBX_SDK includes not found")
endif()

# OpenMP
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# use the file(GLOB...) or file(GLOB_RECURSE...) to grab wildcard src files
file(GLOB_RECURSE SOURCES 	"${CMAKE_SOURCE_DIR}/source/*.cpp"
                            "${CMAKE_SOURCE_DIR}/include/*.h")

include_directories(${CMAKE_SOURCE_DIR}/include ${FBX_PATH})

add_executable(Fbx_Parser ${SOURCES})

# set visual studio startup project
set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
			PROPERTY VS_STARTUP_PROJECT
            Fbx_Parser)

# add libraries
if(UNIX OR MINGW)
    find_library(FBX_LIB libfbxsdk.a
                ${CMAKE_SOURCE_DIR}/FBX_SDK/lib/gcc4/release)
    if(NOT FBX_LIB)
    message(FATAL_ERROR "FBX_SDK gcc Release library not found")
    endif()
    find_library(FBXD_LIB libfbxsdk.a
                ${CMAKE_SOURCE_DIR}/FBX_SDK/lib/gcc4/debug)
    if(NOT FBXD_LIB)
    message(FATAL_ERROR "FBX_SDK gcc Debug library not found")
    endif()
elseif(WIN32)
    find_library(FBX_LIB libfbxsdk-md
                 ${CMAKE_SOURCE_DIR}/FBX_SDK/lib/vs2015/release)
    if(NOT FBX_LIB)
      message(FATAL_ERROR "FBX_SDK windows Release library not found")
    endif()
    find_library(FBXD_LIB libfbxsdk-md
                 ${CMAKE_SOURCE_DIR}/FBX_SDK/lib/vs2015/debug)
    if(NOT FBXD_LIB)
      message(FATAL_ERROR "FBX_SDK windows Debug library not found")
    endif()
endif()

if(UNIX OR MINGW)
    target_link_libraries(Fbx_Parser optimized ${FBX_LIB} "-ldl"
                                     debug ${FBXD_LIB} "-ldl")
elseif(WIN32)
    target_link_libraries(Fbx_Parser optimized ${FBX_LIB} debug ${FBXD_LIB})
endif()
