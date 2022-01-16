/* Mandelbrot code
 * By: John Jekel
*/

/* Constants And Defines */

#define ITERATIONS 256//1000
#define CONVERGE_VALUE 2

#define USE_THREADING//FIXME float is broken for some images when using threading
#define PROCESSING_CHUNKS (max_threads * 4)//So that CPUs aren't just left sitting around

/* Includes */

#include "mandelbrot.h"

#include <stdint.h>
#include <stdlib.h>
#include <complex.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#ifdef USE_THREADING

#ifdef __STDC_NO_THREADS__
#error "C11 threading support required for compiling mandelbrot.c"
#endif

#include <threads.h>
#include <stdatomic.h>
#endif

#ifdef __x86_64__
#include <emmintrin.h>
#endif

/* Types */

#ifdef USE_THREADING
typedef struct
{
    uint8_t thread_num;
    mb_intensities_t* intensities;
} thread_workload_t;
#endif

/* Variables */

static uint16_t max_threads = 1;
static atomic_ushort current_threads = 0;

/* Static Function Declarations */

static uint16_t mandelbrot_iterations_basic(complex long double c);

#ifdef __x86_64__
//static __m128i mandelbrot_iterations_sse2_4(__m128 c_real, __m128 c_imag);
#endif

#ifdef USE_THREADING
static int generate_intensities_threaded(void* workload_);
#endif

/* Function Implementations */

void mb_set_total_active_threads(uint16_t threads)
{
    assert(threads > 0);
    max_threads = threads;
}

mb_intensities_t* mb_generate_intensities(const mb_config_t* restrict config)
{
    mb_intensities_t* restrict intensities = (mb_intensities_t*) malloc(sizeof(mb_config_t) + (sizeof(uint16_t) * config->x_pixels * config->y_pixels));
    memcpy(&intensities->config, config, sizeof(mb_config_t));

    //TODO use vectorization

#ifdef USE_THREADING
    thrd_t threads[PROCESSING_CHUNKS];
    thread_workload_t workloads[PROCESSING_CHUNKS];

    for (uint8_t i = 0; i < PROCESSING_CHUNKS; ++i)
    {
        //Wait for our time to begin
        while (current_threads >= max_threads)
        {
            thrd_yield();
        }
        ++current_threads;

        workloads[i].thread_num = i;
        workloads[i].intensities = intensities;
        thrd_create(&threads[i], generate_intensities_threaded, (void*)&workloads[i]);
    }

    for (uint8_t i = 0; i < PROCESSING_CHUNKS; ++i)
    {
        thrd_join(threads[i], NULL);
    }
#else
    const long double x_step = (config->max_x - config->min_x) / config->x_pixels;
    const long double y_step = (config->max_y - config->min_y) / config->y_pixels;

    long double x = config->min_x;

/*
#ifdef __x86_64__
    //TODO

    for (uint16_t i = 0; i < config->x_pixels; i += 4)
    {
        long double y = config->min_y;
        for (uint16_t j = 0; j < config->y_pixels; ++j)
        {
            __m128 c_real = _mm_set_ps();
            __m128 c_imag;

            __m128i results = mandelbrot_iterations_sse2_4(c_real, c_imag);

            uint16_t results_to_keep[16]

            y += y_step;
        }

        x += x_step * 4;
    }

#else
*/

    for (uint16_t i = 0; i < config->x_pixels; ++i)
    {
        long double y = config->min_y;
        for (uint16_t j = 0; j < config->y_pixels; ++j)
        {
            intensities->intensities[i + (j * config->x_pixels)] = mandelbrot_iterations_basic(CMPLXF(x, y));
            y += y_step;
        }

        x += x_step;
    }

//#endif

#endif

    return intensities;
}

void mb_destroy_intensities(mb_intensities_t* restrict intensities)
{
    free(intensities);
}

void mb_render_bw(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_4);//To allow for rle4 compression
    bmp_palette_set_size(bitmap_to_init, 2);
    bmp_palette_colour_set(bitmap_to_init, 0, (palette_colour_t){0, 0, 0});
    bmp_palette_colour_set(bitmap_to_init, 1, (palette_colour_t){0xFF, 0xFF, 0xFF});

    bmp_clear(bitmap_to_init);

    //TODO make this faster
    for (uint16_t i = 0; i < intensities->config.x_pixels; ++i)
    {
        for (uint16_t j = 0; j < intensities->config.y_pixels; ++j)
        {
            uint32_t value = intensities->intensities[i + (j * intensities->config.x_pixels)] == ITERATIONS ? 0 : 1;

            bmp_px_set_1(bitmap_to_init, i, j, value);
        }
    }
}

void mb_render_grey(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_32);

    bmp_clear(bitmap_to_init);

    //TODO make this faster
    for (uint16_t i = 0; i < intensities->config.x_pixels; ++i)
    {
        for (uint16_t j = 0; j < intensities->config.y_pixels; ++j)
        {
            uint32_t value;
            uint32_t intensity = intensities->intensities[i + (j * intensities->config.x_pixels)];

            if (intensity == ITERATIONS)
                value = 0;
            else
            {
                intensity *= 3;//Avoids obvious banding
                value = ~(intensity | (intensity << 8) | (intensity << 16));
            }

            bmp_px_set(bitmap_to_init, i, j, value);
        }
    }
}

void mb_render_grey_8(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_8);
    bmp_palette_set_size(bitmap_to_init, 256);

    //Simple pallete (maps index to brightness)
    for (uint16_t i = 0; i < 256; ++i)
        bmp_palette_colour_set(bitmap_to_init, i, (palette_colour_t) {.r = i, .g = i, .b = i});

    bmp_clear(bitmap_to_init);

    //TODO make this faster
    for (uint16_t i = 0; i < intensities->config.x_pixels; ++i)
    {
        for (uint16_t j = 0; j < intensities->config.y_pixels; ++j)
        {
            uint32_t value;
            uint32_t intensity = intensities->intensities[i + (j * intensities->config.x_pixels)];

            if (intensity == ITERATIONS)
                value = 255;
            else
                value = intensity;

            value = 255 - value;

            bmp_px_set_8(bitmap_to_init, i, j, value);
        }
    }
}

void mb_render_colour(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_24);

    bmp_clear(bitmap_to_init);

    //TODO make this faster
    for (uint16_t i = 0; i < intensities->config.x_pixels; ++i)
    {
        for (uint16_t j = 0; j < intensities->config.y_pixels; ++j)
        {
            uint32_t value;
            uint32_t intensity = intensities->intensities[i + (j * intensities->config.x_pixels)];

            if (intensity == ITERATIONS)
                value = 0;
            else
            {
                switch (intensity % 3)
                {
                    case 0:
                        value = (intensity % 128) << 1;
                        break;
                    case 1:
                        value = (intensity % 128) << 9;
                        break;
                    case 2:
                        value = (intensity % 128) << 17;
                        break;
                }
            }

            bmp_px_set(bitmap_to_init, i, j, value);
        }
    }
}

void mb_render_colour_8(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_8);
    bmp_palette_set_size(bitmap_to_init, 256);

    //Simple pallete (maps index to brightness)
    bmp_palette_colour_set(bitmap_to_init, 0, (palette_colour_t){0, 0, 0});//Special case: Non converge produces zero
    for (uint16_t i = 1; i < 256; ++i)
    {
        palette_colour_t colour = {0, 0, 0};

        const uint8_t max_iterations = 255;
        const uint8_t band_range = max_iterations / 11;

        if (i < band_range)
        {
            colour.r = i;
        }
        else if (i < (2 * band_range))
        {
            colour.r = i;
            colour.g = i / 2;
        }
        else if (i < (3 * band_range))
        {
            colour.r = i;
            colour.g = i;
        }
        else if (i < (4 * band_range))
        {
            colour.r = i / 2;
            colour.g = i;
        }
        else if (i < (5 * band_range))
        {
            colour.g = i;
        }
        else if (i < (6 * band_range))
        {
            colour.g = i;
            colour.b = i / 2;
        }
        else if (i < (7 * band_range))
        {
            colour.g = i;
            colour.b = i;
        }
        else if (i < (8 * band_range))
        {
            colour.g = i / 2;
            colour.b = i;
        }
        else if (i < (9 * band_range))
        {
            colour.b = i;
        }
        else if (i < (10 * band_range))
        {
            colour.r = i / 2;
            colour.b = i;
        }
        else
        {
            colour.r = i;
            colour.b = i;
        }

        bmp_palette_colour_set(bitmap_to_init, i, colour);
    }

    bmp_clear(bitmap_to_init);

    //TODO make this faster/multithreaded?
    for (uint16_t i = 0; i < intensities->config.x_pixels; ++i)
    {
        for (uint16_t j = 0; j < intensities->config.y_pixels; ++j)
        {
            uint32_t value;
            uint32_t intensity = intensities->intensities[i + (j * intensities->config.x_pixels)];

            if (intensity == ITERATIONS)
                value = 255;//value = 0;
            else
                value = intensity;

            value = 255 - value;

            bmp_px_set_8(bitmap_to_init, i, j, value);
        }
    }
}

/* Static Function Implementations */

static uint16_t mandelbrot_iterations_basic(complex long double c)
{
    complex long double z = 0;//z_0 = 0

    for (uint16_t i = 0; i < ITERATIONS; ++i)
    {
        long double real = creal(z);
        long double imag = cimag(z);

        if (((real * real) + (imag * imag)) >= (CONVERGE_VALUE * CONVERGE_VALUE))//Check if abs(z) >= CONVERGE_VALUE
            return i;

        z = (z * z) + c;//z_(n+1) = z_n^2 + c
    }

    return ITERATIONS;//Failed to converge within ITERATIONS iterations
}

/*
static __m128i mandelbrot_iterations_sse2_4(__m128 c_real, __m128 c_imag)
{
    //https://stackoverflow.com/questions/15986390/some-mandelbrot-drawing-routine-from-c-to-sse2
    //https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#techs=SSE,SSE2
    const __m128 four = _mm_set_ps1(4.0);

    __m128i increment = _mm_set_epi32(1, 1, 1, 1);

}
*/

#ifdef USE_THREADING
static int generate_intensities_threaded(void* workload_)
{
    thread_workload_t* workload = (thread_workload_t*) workload_;

    mb_intensities_t* intensities = workload->intensities;
    mb_config_t* config = &intensities->config;

    //TODO fix bug when using low precision long double and threading
    const uint16_t thread_x_px = config->x_pixels / PROCESSING_CHUNKS;
    const long double thread_x_range = (config->max_x - config->min_x) / PROCESSING_CHUNKS;

    const long double x_step = (config->max_x - config->min_x) / config->x_pixels;
    const long double y_step = (config->max_y - config->min_y) / config->y_pixels;

    //TODO use vector hardware
    long double x = config->min_x + (thread_x_range * workload->thread_num) - (10 * x_step);
    for (uint16_t i = thread_x_px * workload->thread_num; i < thread_x_px * (workload->thread_num + 1); ++i)
    {
        long double y = config->min_y;
        for (uint16_t j = 0; j < config->y_pixels; ++j)
        {
            intensities->intensities[i + (j * config->x_pixels)] = mandelbrot_iterations_basic(CMPLXF(x, y));
            y += y_step;
        }

        x += x_step;
    }

    --current_threads;
    return 0;
}
#endif
