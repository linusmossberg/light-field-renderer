cmake_minimum_required(VERSION 3.10)

project(light-field-renderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG(-O3 COMPILER_SUPPORTS_O3)
if(COMPILER_SUPPORTS_O3)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

CHECK_CXX_COMPILER_FLAG(-g COMPILER_SUPPORTS_G)
if(COMPILER_SUPPORTS_G)
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()

set(NANOGUI_BUILD_SHARED   OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON   OFF CACHE BOOL " " FORCE)
set(NANOGUI_INSTALL        OFF CACHE BOOL " " FORCE)

add_subdirectory(lib/nanogui)
include_directories(lib/nanogui/include)
include_directories(${NANOGUI_EXTRA_INCS})
add_definitions(${NANOGUI_EXTRA_DEFS})

include_directories(lib/glm)

file(GLOB_RECURSE _source_list ${PROJECT_SOURCE_DIR}/source/*)

foreach(_source IN ITEMS ${_source_list})
  get_filename_component(_source_path "${_source}" PATH)
  string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
  string(REPLACE "/" "\\" _group_path "${_group_path}")
  source_group("${_group_path}" FILES "${_source}")
endforeach()

add_executable(${PROJECT_NAME} ${_source_list})
target_link_libraries(${PROJECT_NAME} nanogui ${NANOGUI_EXTRA_LIBS})