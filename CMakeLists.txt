cmake_minimum_required(VERSION 3.21)

project(mandelbrot_bmp_generator VERSION 0.5)

set(SOURCES src/main.c src/bmp.c src/mandelbrot.c src/interactive.c src/cmdline.c src/cpp.cpp include/bmp.h include/mandelbrot.h include/interactive.h include/cmdline.h include/cpp.h)

add_executable(mbbmp ${SOURCES})

target_include_directories(mbbmp PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/include)


set_property(TARGET mbbmp PROPERTY C_STANDARD 11)
set_property(TARGET mbbmp PROPERTY CXX_STANDARD 11)

target_link_libraries(mbbmp m pthread)

#https://stackoverflow.com/questions/41361631/optimize-in-cmake-by-default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_C_FLAGS "-Wall -Wextra -g3 -mavx -mavx2 -mavx512f")
#set(CMAKE_C_FLAGS "-Wall -Wextra -g3 -mavx -mavx2 -mfma")
#set(CMAKE_C_FLAGS "-Wall -Wextra -g3 -mavx -mavx2)
#set(CMAKE_C_FLAGS "-Wall -Wextra -g3 -mavx")
set(CMAKE_C_FLAGS "-Wall -Wextra -g3")
#set(CMAKE_C_FLAGS "-Wall -Wextra -g3 -mno-sse2")
set(CMAKE_C_FLAGS_DEBUG "-Og")
set(CMAKE_C_FLAGS_RELEASE "-Ofast -flto=auto -fuse-linker-plugin -floop-nest-optimize -fipa-pta -fno-semantic-interposition -fdevirtualize-at-ltrans -fno-plt -fstdarg-opt -frename-registers -fweb -ftree-vectorize")

if (CMAKE_C_BYTE_ORDER STREQUAL LITTLE_ENDIAN)
    set(MBBMP_LITTLE_ENDIAN 1)
else()
    set(MBBMP_LITTLE_ENDIAN 0)
endif()

configure_file(include/cmake_config.h.in include/cmake_config.h)
