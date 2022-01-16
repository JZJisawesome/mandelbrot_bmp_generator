/* mandelbrot_bmp_generator cmdline options processing code
 * By: John Jekel
*/

/* Includes */

#include "cmdline.h"

#include "mandelbrot.h"
#include "bmp.h"
#include "cpp.h"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Types */

#define NUM_IMAGE_TYPES 4

typedef enum {TYPE_BW, TYPE_GREY, TYPE_COLOUR_8, TYPE_COLOUR, TYPE_INVALID} image_t;

static const struct
{
    const char* str;
    image_t type;
} image_types_table[NUM_IMAGE_TYPES] =
{
    {"bw", TYPE_BW},
    {"grey", TYPE_GREY},
    {"colour_8", TYPE_COLOUR_8},
    {"colour", TYPE_COLOUR}
};

/* Static Function Declarations */

static void print_usage_text(void);
static image_t parse_type(const char* str);
static int32_t parse_file(const char* file_name);

/* Function Implementations */

int32_t cmdline(uint32_t argc, const char* const* argv)
{
    if (argc == 2)
        return parse_file(argv[1]);

    if (argc != 10)
    {
        fputs("Error: Invalid number of arguments (expected 1 or 9)\n", stderr);
        print_usage_text();
        return 1;
    }

    mb_config_t config;
    mb_intensities_t* intensities;

    //TODO error checking

    config.x_pixels = atoi(argv[1]);
    config.y_pixels = atoi(argv[2]);
    config.min_x = strtold(argv[3], NULL);
    config.max_x = strtold(argv[4], NULL);
    config.min_y = strtold(argv[5], NULL);
    config.max_y = strtold(argv[6], NULL);

    uint16_t threads = atoi(argv[7]);
    if (!threads)
        threads = cpp_hw_concurrency();
    mb_set_total_active_threads(threads);

    fprintf(stderr, "Generating %s (%hux%hu pixels) using %hu threads... ", argv[9], config.x_pixels, config.y_pixels, threads);

    intensities = mb_generate_intensities(&config);

    bmp_t render;
    switch (parse_type(argv[8]))
    {
        case TYPE_BW:
            mb_render_bw(intensities, &render);
            bmp_save(&render, argv[9], BI_RGB);
            break;
        case TYPE_GREY:
            mb_render_grey_8(intensities, &render);
            bmp_save(&render, argv[9], BI_RLE8);
            break;
        case TYPE_COLOUR_8:
            mb_render_colour_8(intensities, &render);
            bmp_save(&render, argv[9], BI_RLE8);
            break;
        case TYPE_COLOUR:
            mb_render_colour(intensities, &render);
            bmp_save(&render, argv[9], BI_RGB);
            break;
        default:
            fputs("Error: Invalid image type\n", stderr);
            print_usage_text();
            mb_destroy_intensities(intensities);
            return 1;
    }
    bmp_destroy(&render);

    fputs("done\n", stderr);

    mb_destroy_intensities(intensities);
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
    fputs("threads\tNumber of threads to use (0 for auto)\n", stderr);
    fputs("image_type\tOne of: \"bw\", \"grey\", \"colour_8\", \"colour\"\n", stderr);
    fputs("file_name\tThe file name to write to\n", stderr);

    fputs("\nOr provide a file containing lines each having the arguments above to generate several images\n", stderr);
    fputs("\nOr provide no arguments for an interactive session\n", stderr);
}

static image_t parse_type(const char* str)
{
    for (size_t i = 0; i < NUM_IMAGE_TYPES; ++i)
        if (!strcmp(image_types_table[i].str, str))
            return image_types_table[i].type;

    return TYPE_INVALID;
}

static int32_t parse_file(const char* file_name)
{
    //TODO error checking
    FILE* file = fopen(file_name, "r");

    if (!file)
    {
        fprintf(stderr, "Error: Failed to open \"%s\" for reading\n", file_name);
        return 1;
    }

    while (true)
    {
        //Get rid of leading whitespace
        fscanf(file, " ");

        //Skip lines that begin with hashtags
        char maybe_hashtag = getc(file);
        if (maybe_hashtag == EOF)
            break;
        else if (maybe_hashtag == '#')
        {
            //Skip the current line
            fscanf(file, "%*[^\n]\n");
            continue;
        }
        else
            ungetc(maybe_hashtag, file);


        mb_config_t config;

        uint16_t threads;
        char type_string[16];
        char file_name[1024];//TODO support larger file names

        int result = fscanf(file, "%hu %hu %Lf %Lf %Lf %Lf %hu %15s %1023s \n",
                            &config.x_pixels, &config.y_pixels,
                            &config.min_x, &config.max_x, &config.min_y, &config.max_y,
                            &threads, type_string, file_name);

        if (result == EOF)
            break;

        if (!threads)
            threads = cpp_hw_concurrency();
        mb_set_total_active_threads(threads);

        fprintf(stderr, "Generating %s (%hux%hu pixels) using %hu threads... ", file_name, config.x_pixels, config.y_pixels, threads);

        mb_intensities_t* intensities = mb_generate_intensities(&config);

        bmp_t render;
        switch (parse_type(type_string))
        {
            case TYPE_BW:
                mb_render_bw(intensities, &render);
                bmp_save(&render, file_name, BI_RGB);
                break;
            case TYPE_GREY:
                mb_render_grey_8(intensities, &render);
                bmp_save(&render, file_name, BI_RLE8);
                break;
            case TYPE_COLOUR_8:
                mb_render_colour_8(intensities, &render);
                bmp_save(&render, file_name, BI_RLE8);
                break;
            case TYPE_COLOUR:
                mb_render_colour(intensities, &render);
                bmp_save(&render, file_name, BI_RGB);
                break;
            default:
                fputs("Error: Invalid image type\n", stderr);
                print_usage_text();
                mb_destroy_intensities(intensities);
                return 1;
        }
        bmp_destroy(&render);

        fputs("done\n", stderr);

        mb_destroy_intensities(intensities);
    }

    fclose(file);
    return 0;
}
