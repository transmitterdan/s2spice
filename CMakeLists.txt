cmake_minimum_required(VERSION 3.10)

if (APPLE)
  set(PACKAGE_NAME S2spice)
else (APPLE)
  set(PACKAGE_NAME s2spice)
endif (APPLE)

project(S2pice)

# guard against bad build-type strings

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Find the wxWidgets library
SET(wxWidgets_USE_LIBS base core)
find_package(wxWidgets REQUIRED)
include(${wxWidgets_USE_FILE})

# Add the source files
add_executable(${PACKAGE_NAME}
    s2spice.cpp
)
target_include_directories(s2spice PUBLIC ~/eigen/Eigen/)
# Link the executable to the wxWidgets library
target_link_libraries(s2spice ${wxWidgets_LIBRARIES})
