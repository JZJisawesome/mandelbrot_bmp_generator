cmake_minimum_required(VERSION 3.10)

project(mandelbrot_bmp_generator)

add_executable(mbbmp src/main.c src/bmp.c src/mandelbrot.c)
