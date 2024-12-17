cmake_minimum_required(VERSION 3.10)

project(engate)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# These aren't used yet, but will be there if needed
set(EG_ARCH "" CACHE STRING "Cross-compilation target architecture (empty for native, arm64, or armhf)")
set_property(CACHE EG_ARCH PROPERTY STRINGS "" "arm64" "armhf")

if(NOT DEFINED PLATFORM_SUFFIX)
  set(PLATFORM_SUFFIX "")
endif()

# Include directories for libvpx and libwebm
include_directories(${CMAKE_SOURCE_DIR}/libvpx)
include_directories(${CMAKE_SOURCE_DIR}/libwebm)
include_directories(${CMAKE_SOURCE_DIR}/libyuv/include)
include_directories(${CMAKE_SOURCE_DIR}/libopus/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES src/*.c src/*.cc)
file(GLOB_RECURSE HEADERS src/*.h src/*.hh include/*.h)

add_custom_command(
    OUTPUT libengate.a
    DEPENDS engate_only
    VERBATIM
    COMMAND rm libengate.a || true
    COMMAND ${CMAKE_AR} rcsT libengate.a $<TARGET_FILE:engate_only>
      ${CMAKE_SOURCE_DIR}/libwebm/build${PLATFORM_SUFFIX}/libwebm.a
      ${CMAKE_SOURCE_DIR}/libvpx/build${PLATFORM_SUFFIX}/libvpx.a 
      ${CMAKE_SOURCE_DIR}/libopus/build${PLATFORM_SUFFIX}/libopus.a 
      ${CMAKE_SOURCE_DIR}/libyuv/build${PLATFORM_SUFFIX}/libyuv.a
    COMMAND ${CMAKE_AR} -s libengate.a
)

add_custom_target(engate ALL DEPENDS libengate.a)

add_library(engate_only STATIC ${SOURCES} ${HEADERS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,address")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

# Link the static libraries
target_link_libraries(engate_only m pthread)

# Include directories in the target
target_include_directories(engate_only PRIVATE ${CMAKE_SOURCE_DIR}/libvpx ${CMAKE_SOURCE_DIR}/libwebm)

file(GLOB_RECURSE DEMO_SOURCES demo/*.c demo/*.cc)
file(GLOB_RECURSE DEMO_HEADERS demo/*.c demo/*.cc demo/*.h demo/*.hh)

add_executable(engate_demo ${DEMO_SOURCES} ${DEMO_HEADERS})
add_dependencies(engate_demo engate)
target_include_directories(engate_demo PRIVATE demo)
target_link_directories(engate_demo PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(engate_demo libengate.a m pthread)
