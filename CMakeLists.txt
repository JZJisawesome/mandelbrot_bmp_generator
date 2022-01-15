cmake_minimum_required(VERSION 3.21)

project(mandelbrot_bmp_generator)

set(SOURCES src/main.c src/bmp.c src/mandelbrot.c include/bmp.h include/mandelbrot.h include/cmake_config.h.in)

add_executable(mbbmp ${SOURCES})

configure_file(include/cmake_config.h.in include/cmake_config.h)

target_include_directories(mbbmp PRIVATE include)

set_property(TARGET mbbmp PROPERTY C_STANDARD 23)

target_link_libraries(mbbmp m pthread)
