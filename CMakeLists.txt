cmake_minimum_required(VERSION 3.17)
project(wzlibcpp)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(3rdparty/zlib)
add_subdirectory(3rdparty/mio)
add_subdirectory(3rdparty/AES)


file(GLOB SOURCE_FILES src/*.cpp)
#file(GLOB AES_SOURCE_FILES AES/AES.cpp)

add_library(wzlib STATIC ${SOURCE_FILES})

target_link_libraries(wzlib PUBLIC zlibstatic mio::mio AES)

target_include_directories(wzlib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/wz
)


if (APPLE)
    project(wzlibtest)

    add_executable(wzlibtest main/main.cpp ${SOURCE_FILES})
    target_link_libraries(
            wzlibtest
            PRIVATE
            wzlib
            zlibstatic
    )
endif ()