cmake_minimum_required(VERSION 3.21)

project(mandelbrot_bmp_generator VERSION 0.1)

set(SOURCES src/main.c src/bmp.c src/mandelbrot.c src/interactive.c src/cmdline.c include/bmp.h include/mandelbrot.h include/interactive.h include/cmdline.h)

add_executable(mbbmp ${SOURCES})

configure_file(include/cmake_config.h.in include/cmake_config.h)

target_include_directories(mbbmp PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/include)


set_property(TARGET mbbmp PROPERTY C_STANDARD 23)

target_link_libraries(mbbmp m pthread)

#https://stackoverflow.com/questions/41361631/optimize-in-cmake-by-default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-Ofast -flto=auto -fuse-linker-plugin -floop-nest-optimize -fipa-pta -fno-semantic-interposition -fdevirtualize-at-ltrans -fno-plt -fstdarg-opt -frename-registers -fweb -ftree-vectorize")
