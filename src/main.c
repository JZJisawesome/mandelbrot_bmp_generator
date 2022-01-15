/* NAME//TODO
 * By: John Jekel
 *
 * TODO description
 *
*/

/* Includes */

#include "mandelbrot.h"
#include "bmp.h"
#include "cmake_config.h"

#include <stdio.h>
#include <threads.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>

/* Constants And Defines */

static const mb_config_t mandelbrot_config =
{
    .x_pixels = 960,//3840,//3840 * 4
    .y_pixels = 540,//2160,//2160 * 4

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

typedef struct
{
    mb_config_t config;
    mb_intensities_t* intensities;
} interactive_intensity_gen_struct_t;

/* Variables */

//TODO

/* Static Function Declarations */

/*
int bw_render_thread(void* intensities_void_ptr);
int grey8_render_thread(void* intensities_void_ptr);
int colour8_render_thread(void* intensities_void_ptr);
*/

static void interactive(void);
static uint16_t interactive_prompt_for_uint(const char* str);
static long double interactive_prompt_for_ld(const char* str);
static char* interactive_prompt_str(const char* str);
static bool interactive_prompt_yn(const char* str);
static int interactive_generate_intensities(void* interactive_intensity_gen_struct);

static void cmdline(int argc, char** argv);

/* Function Implementations */

int main(int argc, char** argv)
{
    printf("mandelbrot_bmp_generator version %u.%u\n", MBBMP_VERSION_MAJOR, MBBMP_VERSION_MINOR);
    printf("Compiled on %s at %s\n", __DATE__, __TIME__);

    if (argc < 2)//Interactive session
        interactive();
    else
        cmdline(argc, argv);

    return 0;

    /*
    mb_intensities_t* mandelbrot_intensities = mb_generate_intensities(&mandelbrot_config);

    //Render in black in white
    thrd_t render_bw;
    thrd_create(&render_bw, bw_render_thread, mandelbrot_intensities);

    //Render in greyscale
    bmp_t render_grey;
    mb_render_grey(mandelbrot_intensities, &render_grey);
    bmp_save(&render_grey, "/tmp/render_grey.bmp", BI_RGB);
    bmp_destroy(&render_grey);

    //Render in greyscale (8 bit, more space efficient)
    thrd_t render_grey8;
    thrd_create(&render_grey8, grey8_render_thread, mandelbrot_intensities);

    //Render in colour
    bmp_t render_colour;
    mb_render_colour(mandelbrot_intensities, &render_colour);
    bmp_save(&render_colour, "/tmp/render_colour.bmp", BI_RGB);
    bmp_destroy(&render_colour);

    //Render in colour (8 bit, more space efficient)
    thrd_t render_colour8;
    thrd_create(&render_colour8, colour8_render_thread, mandelbrot_intensities);

    thrd_join(render_bw, NULL);
    thrd_join(render_grey8, NULL);
    thrd_join(render_colour8, NULL);
    mb_destroy_intensities(mandelbrot_intensities);
    */
    return 0;
}

/* Static Function Implementations */

static void interactive(void)
{
    interactive_intensity_gen_struct_t async_struct;

    //Prompt for info needed to generate intensities
    async_struct.config.x_pixels = interactive_prompt_for_uint("Enter the x resolution of the image (positive integer < 65536): ");
    async_struct.config.y_pixels = interactive_prompt_for_uint("Enter the y resolution of the image (positive integer < 65536): ");
    async_struct.config.min_x = interactive_prompt_for_ld("Enter the lower real bound of the fractal to produce: ");
    async_struct.config.max_x = interactive_prompt_for_ld("Enter the upper real bound of the fractal to produce: ");
    async_struct.config.min_y = interactive_prompt_for_ld("Enter the lower imaginary bound of the fractal to produce: ");
    async_struct.config.max_y = interactive_prompt_for_ld("Enter the upper imaginary bound of the fractal to produce: ");
    mb_set_total_active_threads(interactive_prompt_for_uint("Enter the number of threads to use (positive integer < 65536): "));

    //Generate intensities while we prompt for other info
    thrd_t intensity_gen_thread;
    thrd_create(&intensity_gen_thread, &interactive_generate_intensities, (void*) &async_struct);

    //bool


    //Wait for the intensities to finish generating, then async_struct.intensities will be valid!
    thrd_join(intensity_gen_thread, NULL);

    //TESTING
    bmp_t render;
    mb_render_grey_8(async_struct.intensities, &render);
    bmp_save(&render, "render_grey_8.bmp", BI_RLE8);
    bmp_destroy(&render);

    mb_destroy_intensities(async_struct.intensities);
}

static uint16_t interactive_prompt_for_uint(const char* str)
{
    const size_t input_buffer_size = 64;//Should be enough for a whole int64_t
    char input_buffer[input_buffer_size];
    int64_t input_int;
    do
    {
        fputs(str, stdout);
        fflush(stdout);
        fgets(input_buffer, input_buffer_size, stdin);
        input_int = atol(input_buffer);

    } while ((input_int < 1) || (input_int > UINT_MAX));

    return (uint16_t) input_int;
}

static long double interactive_prompt_for_ld(const char* str)
{
    const size_t input_buffer_size = 64;//Enough precision for 128 bit floats
    char input_buffer[input_buffer_size];
    long double input_ld;

    fputs(str, stdout);
    fflush(stdout);
    fgets(input_buffer, input_buffer_size, stdin);
    input_ld = strtold(input_buffer, NULL);

    return input_ld;
}

static bool interactive_prompt_yn(const char* str)
{
    const size_t input_buffer_size = 64;
    char input_buffer[input_buffer_size];

    while (true)
    {
        fputs(str, stdout);
        fflush(stdout);
        fgets(input_buffer, input_buffer_size, stdin);

        for (size_t i = 0; i < input_buffer_size; ++i)
        {
            if (isspace(input_buffer[i]))
                continue;

            switch (input_buffer[i])
            {
                case 'y':
                case 'Y':
                    return true;
                case 'n':
                case 'N':
                    return false;
                default://Invalid input
                    i = input_buffer_size;
                    break;
            }
        }
    }
}

static int interactive_generate_intensities(void* interactive_intensity_gen_struct)
{
    interactive_intensity_gen_struct_t* async_struct = (interactive_intensity_gen_struct_t*) interactive_intensity_gen_struct;
    async_struct->intensities = mb_generate_intensities(&async_struct->config);
    return 0;
}

static void cmdline(int argc, char** argv)
{
    assert(false);//TODO implement
}

/*
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
*/
