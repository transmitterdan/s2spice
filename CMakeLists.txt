cmake_minimum_required(VERSION 3.22)

# REM On Windows use this to invoke cmake
# cd build
# cmake -AWIN32 -G "Visual Studio 17 2022" ..
# REM  build release and debug versions
# cmake --build . --config Release && cmake --build . --config Debug

# All platforms need the eigen template library
# git clone https://github.com/libigl/eigen

if (APPLE)
  set(PACKAGE_NAME S2spice)
else (APPLE)
  set(PACKAGE_NAME s2spice)
endif (APPLE)

project(${PACKAGE_NAME} LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# set the project version. 
set(PROJECT_VER_PATCH 3)
set(PROJECT_VER_MINOR 0)
set(PROJECT_VER_MAJOR 1)

set(PROJECT_VER "${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}.${PROJECT_VER_PATCH}")
message(STATUS "Building version ${PROJECT_VER}")

set(CMAKE_CXX_STANDARD 11)
message(STATUS "Setting C++11 standard via cmake standard mechanism")

# guard against bad build-type strings

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

file(WRITE "${CMAKE_SOURCE_DIR}/version.h"
  "// version.h\n")
file(APPEND "${CMAKE_SOURCE_DIR}/version.h"
  "#if !defined(__VERSION__S2SPICE__)\n")
file(APPEND "${CMAKE_SOURCE_DIR}/version.h"
  "#define __VERSION__S2SPICE__\n")
file(APPEND "${CMAKE_SOURCE_DIR}/version.h"
  "const string versionString(\"${PROJECT_VER_MAJOR}.${PROJECT_VER_MINOR}.${PROJECT_VER_PATCH}\");\n")
file(APPEND "${CMAKE_SOURCE_DIR}/version.h"
  "#endif\n")

file(WRITE "${CMAKE_SOURCE_DIR}/version.bat"
  "REM version.bat\n")
file(APPEND "${CMAKE_SOURCE_DIR}/version.bat"
  "set \"VERSION_STRING=${PROJECT_VER}\"\n")
file(APPEND "${CMAKE_SOURCE_DIR}/version.bat"
  "exit /b 0\n")

if (MSVC)
  if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PACKAGE_NAME})
  endif (CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
endif (MSVC)

set(wxWidgets_USE_UNICODE ON)
set(wxWidgets_USE_UNIVERSAL OFF)
set(wxWidgets_USE_STATIC OFF)

# Find the wxWidgets library
SET(wxWidgets_USE_LIBS base core)
find_package(wxWidgets REQUIRED)
include(${wxWidgets_USE_FILE})

# Add the source files
if (WIN32)
  if (MSVC)
    add_executable (${PACKAGE_NAME} WIN32 main.cpp ${PACKAGE_NAME}.rc)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PACKAGE_NAME})
    install(TARGETS ${PACKAGE_NAME} RUNTIME
      DESTINATION bin COMPONENT binaries)
    install(FILES ${CMAKE_SOURCE_DIR}/wxWidgets-3.2.1/lib/vc14x_dll/wxbase32u_vc14x.dll;
                  ${CMAKE_SOURCE_DIR}/wxWidgets-3.2.1/lib/vc14x_dll/wxmsw32u_core_vc14x.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/MSVCP140.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/VCRUNTIME140.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/api-ms-win-crt-heap-l1-1-0.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/api-ms-win-crt-runtime-l1-1-0.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/api-ms-win-crt-string-l1-1-0.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/api-ms-win-crt-convert-l1-1-0.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/api-ms-win-crt-stdio-l1-1-0.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/api-ms-win-crt-filesystem-l1-1-0.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/api-ms-win-crt-math-l1-1-0.dll
                  ${CMAKE_SOURCE_DIR}/win_dep/api-ms-win-crt-locale-l1-1-0.dll
            DESTINATION bin COMPONENT binaries)
    file(GLOB TEST_LIST ${CMAKE_SOURCE_DIR}/Test/*.s?p
                        ${CMAKE_SOURCE_DIR}/Test/*.asc
                        ${CMAKE_SOURCE_DIR}/Test/*.plt
                        ${CMAKE_SOURCE_DIR}/license.*
        )
    install(FILES ${TEST_LIST}
      DESTINATION Test COMPONENT data_files)
  endif (MSVC)
else (WIN32)
  add_executable (${PACKAGE_NAME} main.cpp)
endif (WIN32)

add_library(SObject STATIC 
    SObject.cpp
    SObject.h
    xqsmatrix.h)

# Link the executable to the wxWidgets library
target_link_libraries(${PACKAGE_NAME} ${wxWidgets_LIBRARIES} SObject)

include(CpackConfig)
