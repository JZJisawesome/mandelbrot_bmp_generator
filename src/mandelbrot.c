/* NAME//TODO
 * By: John Jekel
 *
 * TODO description
 *
*/

/* Constants And Defines */

#define ITERATIONS 256//1000
#define CONVERGE_VALUE 2
#define fp_t double//FIXME float is broken for some images when using threading

#define USE_THREADING
#define THREADS 12
#define PROCESSING_CHUNKS (THREADS * 4)//So that CPUs aren't just left sitting around

/* Includes */

#include "mandelbrot.h"

#include <stdint.h>
#include <stdlib.h>
#include <complex.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>//TESTING

#ifdef USE_THREADING
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
    atomic_ushort* active_threads;
} thread_workload_t;
#endif

/* Variables */

//TODO

/* Static Function Declarations */

static uint16_t mandelbrot_iterations_basic(complex fp_t c);

#ifdef __x86_64__
static __m128i mandelbrot_iterations_sse2_4(__m128 c_real, __m128 c_imag);
#endif

#ifdef USE_THREADING
static int generate_intensities_threaded(void* workload_);
#endif

/* Function Implementations */

mb_intensities_t* mb_generate_intensities(const mb_config_t* restrict config)
{
    mb_intensities_t* restrict intensities = (mb_intensities_t*) malloc(sizeof(mb_config_t) + (sizeof(uint16_t) * config->x_pixels * config->y_pixels));
    memcpy(&intensities->config, config, sizeof(mb_config_t));

    //TODO use vectorization

#ifdef USE_THREADING
    atomic_ushort active_threads = 0;
    thrd_t threads[PROCESSING_CHUNKS];
    thread_workload_t workloads[PROCESSING_CHUNKS];

    for (uint8_t i = 0; i < PROCESSING_CHUNKS; ++i)
    {
        //Wait for our time to begin
        while (active_threads >= THREADS)
        {
            thrd_yield();
        }
        ++active_threads;

        workloads[i].thread_num = i;
        workloads[i].intensities = intensities;
        workloads[i].active_threads = &active_threads;
        thrd_create(&threads[i], generate_intensities_threaded, (void*)&workloads[i]);
    }

    for (uint8_t i = 0; i < PROCESSING_CHUNKS; ++i)
    {
        thrd_join(threads[i], NULL);
    }
#else
    const fp_t x_step = (config->max_x - config->min_x) / config->x_pixels;
    const fp_t y_step = (config->max_y - config->min_y) / config->y_pixels;

    fp_t x = config->min_x;

/*
#ifdef __x86_64__
    //TODO

    for (uint16_t i = 0; i < config->x_pixels; i += 4)
    {
        fp_t y = config->min_y;
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
        fp_t y = config->min_y;
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

static uint16_t mandelbrot_iterations_basic(complex fp_t c)
{
    complex fp_t z = 0;//z_0 = 0

    for (uint16_t i = 0; i < ITERATIONS; ++i)
    {
        fp_t real = creal(z);
        fp_t imag = cimag(z);

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

    //TODO fix bug when using low precision fp_t and threading
    const uint16_t thread_x_px = config->x_pixels / PROCESSING_CHUNKS;
    const fp_t thread_x_range = (config->max_x - config->min_x) / PROCESSING_CHUNKS;

    const fp_t x_step = (config->max_x - config->min_x) / config->x_pixels;
    const fp_t y_step = (config->max_y - config->min_y) / config->y_pixels;

    //TODO use vector hardware
    fp_t x = config->min_x + (thread_x_range * workload->thread_num) - (10 * x_step);
    for (uint16_t i = thread_x_px * workload->thread_num; i < thread_x_px * (workload->thread_num + 1); ++i)
    {
        fp_t y = config->min_y;
        for (uint16_t j = 0; j < config->y_pixels; ++j)
        {
            intensities->intensities[i + (j * config->x_pixels)] = mandelbrot_iterations_basic(CMPLXF(x, y));
            y += y_step;
        }

        x += x_step;
    }

    --(*workload->active_threads);
    return 0;
}
#endif
