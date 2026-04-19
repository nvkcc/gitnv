cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0135 OLD)

project(git-nv C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-O3")
set(CMAKE_INSTALL_PREFIX /usr/local)

# Set libgit2 options (e.g. disable shared lib)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR OFF CACHE BOOL "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)

add_executable(git-nv main.c)
set_property(TARGET git-nv PROPERTY C_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
  libgit2
  URL https://github.com/libgit2/libgit2/archive/ca225744b992bf2bf24e9a2eb357ddef78179667.zip) # v1.9.2
# FetchContent_Declare(
#   libgit2
#   GIT_REPOSITORY https://github.com/libgit2/libgit2.git
#   GIT_TAG        ca225744b992bf2bf24e9a2eb357ddef78179667) # v1.9.2
FetchContent_MakeAvailable(libgit2)

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(Yellow      "${Esc}[33m")
endif()

message(STATUS "${Yellow}LibGit2 source dir: ${libgit2_SOURCE_DIR}${ColourReset}")
message(STATUS "${Yellow}LibGit2 libraries: ${libgit2_LIBRARIES}${ColourReset}")

# Print all accessible variables.
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${Yellow}${_variableName}=${${_variableName}}${ColourReset}")
endforeach()

target_include_directories(git-nv PRIVATE "${libgit2_SOURCE_DIR}/include" ${libgit2_SOURCE_DIR})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(git-nv PUBLIC DEBUG)
    string(ASCII 27 Esc)
    message(STATUS "${Yellow}Building git-nv in debug mode.${ColourReset}")
endif()

# Create compile_commands.json for clangd to latch on to.
file(CREATE_LINK ${CMAKE_BINARY_DIR}/compile_commands.json compile_commands.json SYMBOLIC)

# Install.
install(TARGETS git-nv)
