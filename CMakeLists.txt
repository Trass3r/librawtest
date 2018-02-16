cmake_minimum_required(VERSION 3.9)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "")

project(librawtest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/bin)

add_library(libraw SHARED IMPORTED)
set_target_properties(libraw PROPERTIES
	IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/LibRaw-0.18.7/bin/libraw.dll
	IMPORTED_IMPLIB  ${CMAKE_CURRENT_SOURCE_DIR}/LibRaw-0.18.7/lib/libraw.lib
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/LibRaw-0.18.7/libraw
)
# TODO: INTERFACE_SYSTEM_INCLUDE_DIRECTORIES not working in VS

add_executable(librawtest main.cpp)
target_compile_options(librawtest PRIVATE -wd4996 -wd4251)
target_link_libraries(librawtest libraw ${CMAKE_CURRENT_SOURCE_DIR}/demosaic.obj)
