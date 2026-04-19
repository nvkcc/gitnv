cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 OLD)

project(git-nv C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-O3")
set(CMAKE_INSTALL_PREFIX /usr/local)

# C++ options (mostly for Google Test)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# libgit2 options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR OFF CACHE BOOL "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)

# ==============================================================================
# External Dependencies.

include(FetchContent)

FetchContent_Declare(
  libgit2
  URL https://github.com/libgit2/libgit2/archive/ca225744b992bf2bf24e9a2eb357ddef78179667.zip # v1.9.2
)
FetchContent_Declare(
  cwalk
  URL https://github.com/likle/cwalk/archive/f45a23a13abf39d94b347d7c83810eca26a5a8d0.zip # v1.2.9
)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip # 1.17.0
)

# (Google Test) For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(libgit2 cwalk googletest)
# ==============================================================================

include("./.github/colors.cmake")

set(GITNV_SOURCES
    "${CMAKE_SOURCE_DIR}/kstr.c"
    "${CMAKE_SOURCE_DIR}/state.c"
)

add_executable(git-nv "${CMAKE_SOURCE_DIR}/main.c" ${GITNV_SOURCES})

# Define the macro `#define DEBUG` if the build type is set to "Debug".
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(git-nv PUBLIC DEBUG)
    message(STATUS "${Yellow}Building git-nv in debug mode.${ColourReset}")
endif()

target_include_directories(git-nv PRIVATE "${libgit2_SOURCE_DIR}/include")
target_link_libraries(git-nv PRIVATE libgit2package cwalk)

# Create compile_commands.json for clangd to latch on to.
file(CREATE_LINK ${CMAKE_BINARY_DIR}/compile_commands.json compile_commands.json SYMBOLIC)

# Install.
install(TARGETS git-nv)
