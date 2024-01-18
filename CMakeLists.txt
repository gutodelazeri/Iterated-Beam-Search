cmake_minimum_required(VERSION  3.5)

project(fire CXX)

option(DEBUG "Debug mode." OFF)

# Build definitions
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wno-unknown-pragmas -fPIC -DIL_STD -fexceptions -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -Wall -Wno-unknown-pragmas -fPIC -DIL_STD -fexceptions")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


# Modules
include(FetchContent)

# External
find_package(Boost 1.70.0 COMPONENTS program_options timer system chrono filesystem)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

# Global variables
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
include_directories(${Boost_INCLUDE_DIRS} ${SOURCE_DIR}
                                          ${SOURCE_DIR}/algorithm
                                          ${SOURCE_DIR}/problem
                                          ${SOURCE_DIR}/util)

# Build main
add_executable(${CMAKE_PROJECT_NAME}  ${SOURCE_DIR}/main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json 
                                                    ${Boost_LIBRARIES})
