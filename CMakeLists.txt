cmake_minimum_required(VERSION 3.25)
include(FetchContent)

find_package(PkgConfig REQUIRED)

set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -Wall -Wextra -Wpedantic -Wno-psabi -std=c++20")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(BUILD_SHARED_LIBS OFF)

project(mikugl CXX C)
set(FETCHCONTENT_QUIET FALSE)

set(LIB_INCLUDE)
set(LIB_LINK)
set(LIB_SOURCES)

option(MIKU_BUILD_TESTS "Build tests" ON)
if (MIKU_BUILD_TESTS)
  enable_testing()
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
  )
  FetchContent_MakeAvailable(Catch2)
  set(RAN_TESTS ON CACHE BOOL "Build ranmath tests" FORCE)
endif()

# libfmt
find_package(fmt)
list(APPEND LIB_INCLUDE ${FMT_INCLUDE_DIRS})
list(APPEND LIB_LINK fmt::fmt)

# ntfstl
add_subdirectory("lib/ntfstl")
list(APPEND LIB_LINK ntfstl::ntfstl)

# ranmath
add_subdirectory("lib/ranmath")
list(APPEND LIB_LINK ranmath::ranmath)

# mmdgltf
set(MMDLGTF_BUILD_CLI OFF)
add_subdirectory("lib/mmdgltf")
list(APPEND LIB_LINK mmdgltf)

# sdl
find_package(SDL2 REQUIRED CONFIG)
list(APPEND LIB_INCLUDE ${SDL2_INCLUDE_DIRS})
list(APPEND LIB_LINK SDL2::SDL2)

# glad
list(APPEND LIB_INCLUDE "lib/glad/include")
list(APPEND LIB_SOURCES "lib/glad/src/glad.c")

# stb
list(APPEND LIB_INCLUDE "lib/stb/include")

# imgui
file(GLOB IMGUI_SOURCES "lib/imgui/*.cpp")
list(APPEND IMGUI_SOURCES "lib/imgui/backends/imgui_impl_sdl2.cpp")
list(APPEND IMGUI_SOURCES "lib/imgui/backends/imgui_impl_opengl3.cpp")
list(APPEND LIB_INCLUDE "lib/imgui" "lib/imgui/backends")
list(APPEND LIB_SOURCES ${IMGUI_SOURCES})

# project library & executable
file(GLOB_RECURSE LIB_PROJECT_SOURCES "src/**.cpp")
list(REMOVE_ITEM LIB_PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
file(GLOB_RECURSE HEADER_FILES "src/**.hpp")

add_library(mikugl_lib STATIC ${LIB_PROJECT_SOURCES} ${LIB_SOURCES} ${HEADER_FILES})
target_include_directories(mikugl_lib PUBLIC src ${LIB_INCLUDE})
set_target_properties(mikugl_lib PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20)
target_link_libraries(mikugl_lib PUBLIC ${LIB_LINK})

add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC src ${LIB_INCLUDE})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} PRIVATE mikugl_lib)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  -DMIKU_RES_DIR=\"${RES_DIR}\"
  -DMIKU_MMDGLTF_MODEL_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/lib/mmdgltf/models\"
)

if (MIKU_BUILD_TESTS)
  add_subdirectory(test)
endif()
