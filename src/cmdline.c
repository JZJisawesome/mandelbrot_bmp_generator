/* mandelbrot_bmp_generator cmdline options processing code
 * By: John Jekel
*/

/* Constants And Defines */

//TODO

/* Includes */

#include "cmdline.h"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

/* Types */

//TODO

/* Variables */

//TODO

/* Static Function Declarations */

static void print_usage_text(void);

/* Function Implementations */

int32_t cmdline(uint32_t argc, const char* const* argv)
{
    if (argc != 10)
    {
        fputs("Error: Invalid number of arguments (expected 9)\n\n", stderr);
        print_usage_text();
        return 1;
    }




    assert(false);//TODO implement
    return 0;
}

/* Static Function Implementations */

static void print_usage_text(void)
{
    fputs("Usage: mbbmp x_pixels y_pixels min_real max_real min_imag max_imag threads image_type file_name\n\n", stderr);

    fputs("x_pixels\tPixels in the x direction to render\n", stderr);
    fputs("x_pixels\tPixels in the y direction to render\n", stderr);
    fputs("min_real\tLower real bound of the fractal to produce\n", stderr);
    fputs("max_real\tUpper real bound of the fractal to produce\n", stderr);
    fputs("min_imag\tLower imaginary bound of the fractal to produce\n", stderr);
    fputs("max_imag\tUpper imaginary bound of the fractal to produce\n", stderr);
    fputs("threads\tNumber of threads to use\n", stderr);
    fputs("image_type\tOne of: \"bw\", \"grey\", \"colour8\", \"colour\"\n", stderr);
    fputs("file_name\tThe file name to write to\n", stderr);

    fputs("\nOr provide no arguments for an interactive session\n", stderr);
}
