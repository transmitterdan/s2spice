cmake_minimum_required(VERSION 3.20)

# On Windows use this to invoke cmake
# cd build
# cmake .. -DCMAKE_BUILD_TYPE:STRING=Release -DWX_LIB_DIR="%wxWidgets_LIB_DIR%"

# All platforms need the eigen template library
# git clone https://github.com/libigl/eigen

if (APPLE)
  set(PACKAGE_NAME S2spice)
else (APPLE)
  set(PACKAGE_NAME s2spice)
endif (APPLE)

project(${PACKAGE_NAME} LANGUAGES CXX)

# guard against bad build-type strings

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if (MSVC)
  set(wxWidgets_USE_UNICODE ON)
  set(wxWidgets_USE_UNIVERSAL OFF)
  set(wxWidgets_USE_STATIC ON)
endif (MSVC)

# Find the wxWidgets library
SET(wxWidgets_USE_LIBS base core)
find_package(wxWidgets REQUIRED)
include(${wxWidgets_USE_FILE})

# Add the source files
if (MSVC)
  add_executable (${PACKAGE_NAME} WIN32 main.cpp)
else (MSVC)
  add_executable (${PACKAGE_NAME} main.cpp)
endif (MSVC)

add_library(SObject STATIC
    SObject.cpp
    SObject.h
    xqsmatrix.h)

# Link the executable to the wxWidgets library
target_link_libraries(${PACKAGE_NAME} ${wxWidgets_LIBRARIES} SObject)

install(TARGETS ${PACKAGE_NAME} RUNTIME DESTINATION bin)
include(CPack)
