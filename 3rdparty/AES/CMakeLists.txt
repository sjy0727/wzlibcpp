cmake_minimum_required(VERSION 2.8)

project(AES)

#enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIR})
#

#add_executable(tests
#    AES.cpp
#    tests.cpp
#    )
#
#target_link_libraries(tests ${GTEST_LIBRARIES} pthread)

file(GLOB AES_SOURCE_FILE AES/*.h AES/*.cpp)

add_library(AES STATIC ${AES_SOURCE_FILE})
target_include_directories(AES PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})