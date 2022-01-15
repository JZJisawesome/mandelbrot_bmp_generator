cmake_minimum_required(VERSION 3.21)

project(mandelbrot_bmp_generator VERSION 0.1)

set(SOURCES src/main.c src/bmp.c src/mandelbrot.c include/bmp.h include/mandelbrot.h)

add_executable(mbbmp ${SOURCES})

configure_file(include/cmake_config.h.in include/cmake_config.h)

target_include_directories(mbbmp PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/include)


set_property(TARGET mbbmp PROPERTY C_STANDARD 23)

target_link_libraries(mbbmp m pthread)
