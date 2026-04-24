cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 OLD)

project(git-nv LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # this line is so that we get gnu17 -> get fdopen().
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-O3")
set(CMAKE_INSTALL_PREFIX /usr/local)

# C++ options (mostly for Google Test)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# libgit2 options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR     OFF CACHE BOOL "" FORCE)
set(BUILD_CLI      OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS  OFF CACHE BOOL "" FORCE)
set(USE_NSEC       OFF CACHE BOOL "" FORCE)

option(BUILD_GITNV_TESTS "Build tests for git-nv" OFF)

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
  rxi-log
  GIT_REPOSITORY https://github.com/nvkcc/log.c
  GIT_TAG        a91f7ec797fc1f8c0cb4f29169d8284377fa430f
  DOWNLOAD_EXTRACT_TIMESTAMP false
)
FetchContent_Declare(
  nk_test
  GIT_REPOSITORY https://github.com/nvkcc/nk_test
  GIT_TAG        9b5e86dee066dfce8db73f007f68946adadbe74b
  DOWNLOAD_EXTRACT_TIMESTAMP false
)
# ==============================================================================

set(GITNV_SOURCES
    "${CMAKE_SOURCE_DIR}/state.c"
    "${CMAKE_SOURCE_DIR}/util.c"
    "${CMAKE_SOURCE_DIR}/cache.c"
    "${CMAKE_SOURCE_DIR}/log.c"
)

add_executable(git-nv "${CMAKE_SOURCE_DIR}/main.c" ${GITNV_SOURCES})

# https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#genex:condition
# https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#genex:CONFIG
target_compile_definitions(git-nv PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)

FetchContent_MakeAvailable(libgit2 cwalk rxi-log)
target_link_libraries(git-nv PRIVATE
  general libgit2package
  general cwalk
  debug   rxi-log
)

if(BUILD_GITNV_TESTS)
  # (Google Test) For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(nk_test)
  enable_testing()
  add_executable(git-nv-test "${CMAKE_CURRENT_SOURCE_DIR}/test.cpp" ${GITNV_SOURCES})
  target_compile_definitions(git-nv-test PUBLIC DEBUG_MODE)
  target_link_libraries(git-nv-test PRIVATE libgit2package cwalk nk_test)
  add_test(NAME GitnuTests COMMAND git-nv-test)
else()
  # Install, but only when there are no tests built.
  install(TARGETS git-nv)
endif()
