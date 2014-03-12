cmake_minimum_required(VERSION 2.8)
project(ldig)

set(CMAKE_C_FLAGS_RELEASE "-Wall -O2")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-std=gnu++0x")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/ldig")
include_directories("${PROJECT_SOURCE_DIR}/cybozulib/include")

find_package(Boost 1.46 COMPONENTS regex)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  add_executable(ldig ldig/ldig.cpp)
  target_link_libraries(ldig ${Boost_LIBRARIES})
  add_executable(ldig_test ldigtest/test_model.cpp ldigtest/test_da.cpp)
  target_link_libraries(ldig_test ${Boost_LIBRARIES})
endif()


