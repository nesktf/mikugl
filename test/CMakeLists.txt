cmake_minimum_required(VERSION 3.11)
include(FetchContent)

project(mikugl_test CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -Wextra -Wpedantic -O0 -g2 -ggdb -Wno-psabi")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wall -Wextra -Wpedantic -O3 -Wno-psabi")

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if (NOT TARGET Catch2::Catch2WithMain)
  message(STATUS "mikugl_test: Fetching Catch2 contents...")
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
  )
  FetchContent_MakeAvailable(Catch2)
endif()

set(CATCH_BUILD_TESTING OFF)
enable_testing()

add_executable(${PROJECT_NAME} ${TEST_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../src")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

get_filename_component(MIKUGL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  MIKUGL_TEST_MODEL_PATH="${MIKUGL_ROOT_DIR}/lib/mmdgltf/models/default/Miku_Hatsune_Ver2.pmd"
  MIKUGL_TEST_DATA_DIR="${MIKUGL_ROOT_DIR}/data"
)

target_link_libraries(${PROJECT_NAME} PRIVATE mikugl_lib Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})
