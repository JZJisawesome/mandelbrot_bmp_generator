/* NAME//TODO
 * By: John Jekel
 *
 * TODO description
 *
*/

/* Constants And Defines */

//TODO

/* Includes */

#include "mandelbrot.h"
#include "bmp.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <threads.h>
#include <assert.h>
#include <stdbool.h>

/* Types */

typedef struct
{
    mb_config_t config;
    mb_intensities_t* intensities;
} interactive_intensity_gen_struct_t;

/* Variables */

//TODO

/* Static Function Declarations */

static uint16_t interactive_prompt_for_uint(const char* str);
static long double interactive_prompt_for_ld(const char* str);
static char* interactive_prompt_str(const char* str);
static bool interactive_prompt_yn(const char* str);
static int interactive_generate_intensities(void* interactive_intensity_gen_struct);

/* Function Implementations */

void interactive(void)
{
    interactive_intensity_gen_struct_t async_struct;

    putchar('\n');

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

    char* base_name = interactive_prompt_str("Enter the base file name you wish to create: ");
    bool gen_bw = interactive_prompt_yn("Generate a black and white image? (Y/N): ");
    bool gen_grey = interactive_prompt_yn("Generate a greyscale image? (Y/N): ");
    bool gen_colour_8 = interactive_prompt_yn("Generate an 8-bit colour image? (Y/N): ");
    bool gen_colour = interactive_prompt_yn("Generate a 24-bit colour image? (Y/N): ");

    //Wait for the intensities to finish generating, then async_struct.intensities will be valid!
    thrd_join(intensity_gen_thread, NULL);

    //TODO make this multithreaded + reuse code

    if (gen_bw)
    {
        const char* suffix = "_bw.bmp";
        char* file_name = malloc(sizeof(char) + (strlen(base_name) + strlen(suffix) + 1));
        strcpy(file_name, base_name);
        strcat(file_name, suffix);

        bmp_t render;
        mb_render_bw(async_struct.intensities, &render);
        bmp_save(&render, file_name, BI_RGB);
        bmp_destroy(&render);
        free(file_name);
    }

    if (gen_grey)
    {
        const char* suffix = "_grey.bmp";
        char* file_name = malloc(sizeof(char) + (strlen(base_name) + strlen(suffix) + 1));
        strcpy(file_name, base_name);
        strcat(file_name, suffix);

        bmp_t render;
        mb_render_grey_8(async_struct.intensities, &render);
        bmp_save(&render, file_name, BI_RLE8);
        bmp_destroy(&render);
        free(file_name);
    }

    if (gen_colour_8)
    {
        const char* suffix = "_colour_8.bmp";
        char* file_name = malloc(sizeof(char) + (strlen(base_name) + strlen(suffix) + 1));
        strcpy(file_name, base_name);
        strcat(file_name, suffix);

        bmp_t render;
        mb_render_colour_8(async_struct.intensities, &render);
        bmp_save(&render, file_name, BI_RLE8);
        bmp_destroy(&render);
        free(file_name);
    }

    if (gen_colour)
    {
        const char* suffix = "_colour.bmp";
        char* file_name = malloc(sizeof(char) + (strlen(base_name) + strlen(suffix) + 1));
        strcpy(file_name, base_name);
        strcat(file_name, suffix);

        bmp_t render;
        mb_render_colour(async_struct.intensities, &render);
        bmp_save(&render, file_name, BI_RGB);
        bmp_destroy(&render);
        free(file_name);
    }

    mb_destroy_intensities(async_struct.intensities);
    free(base_name);
}

/* Static Function Implementations */

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

static char* interactive_prompt_str(const char* str)
{
    const size_t input_buffer_size = 1024;
    char* input_buffer = (char*) malloc(sizeof(char) * input_buffer_size);

    fputs(str, stdout);
    fflush(stdout);

    //Clear leading whitespace
    char whitespace;
    while (isspace(whitespace = getchar()));
    ungetc(whitespace, stdin);

    fgets(input_buffer, input_buffer_size, stdin);

    input_buffer[strlen(input_buffer) - 1] = 0x00;//Remove the '\n'
    input_buffer = (char*) realloc(input_buffer, sizeof(char) * (strlen(input_buffer) + 1));
    return input_buffer;
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
