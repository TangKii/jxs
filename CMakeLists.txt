# ============================================================================
# CMake Build Files for JPEG XS Reference Software (jpeg-xsm).
# 
# See LICENSE.md for copyright and licensing conditions.
# ============================================================================

cmake_minimum_required(VERSION 3.9.6)

# Parse version from version.h.
file(READ "libjxs/src/version.h" _VER_FILE)
string(REGEX MATCH "#define[\t ]+JXS_VERSION_MAJOR[\t ]+([0-9]+)" _ ${_VER_FILE})
set(JXS_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define[\t ]+JXS_VERSION_MINOR[\t ]+([0-9]+)" _ ${_VER_FILE})
set(JXS_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define[\t ]+JXS_VERSION_PATCH[\t ]+([0-9]+)" _ ${_VER_FILE})
set(JXS_VERSION_PATCH ${CMAKE_MATCH_1})

message(STATUS "JXS Reference Software (version ${JXS_VERSION_MAJOR}.${JXS_VERSION_MINOR}.${JXS_VERSION_PATCH}).")

# Begin project.
project(jxs_ref_sw VERSION ${JXS_VERSION_MAJOR}.${JXS_VERSION_MINOR}.${JXS_VERSION_PATCH} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Get everything into one final folder.
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

# Some global settings.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if(MSVC)
    # Disable some warnings on MSVC.
    add_compile_definitions(_SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)
endif()

# Handle libm.
find_library(M_LIBRARY NAMES libm.a)
if(M_LIBRARY)
    set(M_LIBRARY "m")
else()
    set(M_LIBRARY "")
endif()

# XS library.
add_subdirectory(libjxs)

# XS Utils library and executables.
add_subdirectory(programs)
