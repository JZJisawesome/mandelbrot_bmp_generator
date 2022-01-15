/* NAME//TODO
 * By: John Jekel
 *
 * TODO description
 *
*/

/* Includes */

#include "mandelbrot.h"
#include "cmake_config.h"

#include <stdio.h>
#include <threads.h>

/* Constants And Defines */

static const mb_config_t mandelbrot_config =
{
    .x_pixels = 3840 * 4,//3840
    .y_pixels = 2160 * 4,//2160

    //Nice spiral
    /*
    .min_x = -0.7463 -0.001,
    .max_x = -0.7463 + 0.003,
    .min_y = 0.1112,
    .max_y = 0.1112 + 0.0035,
    */

    //Whole "zoom out"
    .min_x = -2.3,
    .max_x = 0.8,
    .min_y = -1.1,
    .max_y = 1.1,
};

/* Types */

//TODO

/* Variables */

//TODO

/* Static Function Declarations */

int bw_render_thread(void* intensities_void_ptr);
int grey8_render_thread(void* intensities_void_ptr);
int colour8_render_thread(void* intensities_void_ptr);

/* Function Implementations */

int main()
{
    printf("mandelbrot_bmp_generator version %u.%u\n", MBBMP_VERSION_MAJOR, MBBMP_VERSION_MINOR);
    mb_intensities_t* mandelbrot_intensities = mb_generate_intensities(&mandelbrot_config);

    //Render in black in white
    thrd_t render_bw;
    thrd_create(&render_bw, bw_render_thread, mandelbrot_intensities);

    //Render in greyscale
    /*
    bmp_t render_grey;
    mb_render_grey(mandelbrot_intensities, &render_grey);
    bmp_save(&render_grey, "/tmp/render_grey.bmp", BI_RGB);
    bmp_destroy(&render_grey);
    */

    //Render in greyscale (8 bit, more space efficient)
    thrd_t render_grey8;
    thrd_create(&render_grey8, grey8_render_thread, mandelbrot_intensities);

    //Render in colour
    /*
    bmp_t render_colour;
    mb_render_colour(mandelbrot_intensities, &render_colour);
    bmp_save(&render_colour, "/tmp/render_colour.bmp", BI_RGB);
    bmp_destroy(&render_colour);
    */

    //Render in colour (8 bit, more space efficient)
    thrd_t render_colour8;
    thrd_create(&render_colour8, colour8_render_thread, mandelbrot_intensities);

    thrd_join(render_bw, NULL);
    thrd_join(render_grey8, NULL);
    thrd_join(render_colour8, NULL);
    mb_destroy_intensities(mandelbrot_intensities);
    return 0;
}

/* Static Function Implementations */

int bw_render_thread(void* intensities_void_ptr)
{
    bmp_t render;
    mb_render_bw((mb_intensities_t*)intensities_void_ptr, &render);
    bmp_save(&render, "/tmp/render_bw.bmp", BI_RLE4);
    bmp_destroy(&render);
    return 0;
}

int grey8_render_thread(void* intensities_void_ptr)
{
    bmp_t render;
    mb_render_grey_8((mb_intensities_t*)intensities_void_ptr, &render);
    bmp_save(&render, "/tmp/render_grey_8.bmp", BI_RLE8);
    bmp_destroy(&render);
    return 0;
}

int colour8_render_thread(void* intensities_void_ptr)
{
    bmp_t render;
    mb_render_colour_8((mb_intensities_t*)intensities_void_ptr, &render);
    bmp_save(&render, "/tmp/render_colour_8.bmp", BI_RLE8);
    bmp_destroy(&render);
    return 0;
}
