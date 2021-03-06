/* Mandelbrot code
 * By: John Jekel
*/

/* Constants And Defines */

#define ITERATIONS 255//1000
#define CONVERGE_VALUE 2

#define MBBMP_THREADING

/* Includes */

#include "mandelbrot.h"

#include <stdint.h>
#include <stdlib.h>
#include <complex.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#ifdef MBBMP_THREADING

#ifdef __STDC_NO_THREADS__
#error "C11 threading support required for compiling mandelbrot.c"
#endif

#include <threads.h>
#include <stdatomic.h>
#endif

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __AVX__
#include <immintrin.h>
#endif

/* Types */

#ifdef MBBMP_THREADING
typedef struct
{
    uint8_t thread_num;
    mb_intensities_t* intensities;
} intensity_thread_workload_t;

typedef struct
{
    uint8_t thread_num;
    bmp_t* render;
    const mb_intensities_t* intensities;
} render_thread_workload_t;
#endif

/* Variables */

#ifdef MBBMP_THREADING
static uint16_t max_threads = 1;
static uint16_t processing_chunks = 4;//So that CPUs aren't just left sitting around
static atomic_ushort current_threads = 0;
#endif

/* Static Function Declarations */

static uint16_t mandelbrot_iterations_basic(complex double c);

#ifdef __SSE2__
static __m128i mandelbrot_iterations_sse2_2(__m128d c_real, __m128d c_imag);
#endif

#ifdef __AVX__
static __m256i mandelbrot_iterations_avx_4(__m256d c_real, __m256d c_imag);//This is also for avx2
#endif

static void intensities_render_normal_8(bmp_t* restrict render, const mb_intensities_t* restrict intensities);//TODO implement
static void intensities_render_inverted_8(bmp_t* restrict render, const mb_intensities_t* restrict intensities);//More iterations = darker colour

#ifdef MBBMP_THREADING
static int intensities_render_inverted_8_thread(void* workload_);
static int generate_intensities_threaded(void* workload_);
#endif

/* Function Implementations */

void mb_set_total_active_threads(uint16_t threads)
{
    assert(threads > 0);
#ifdef MBBMP_THREADING
    max_threads = threads;
    processing_chunks = threads * 4;//So that CPUs aren't just left sitting around
#endif
}

mb_intensities_t* mb_generate_intensities(const mb_config_t* restrict config)
{
    mb_intensities_t* restrict intensities = (mb_intensities_t*) malloc(sizeof(mb_config_t) + (sizeof(uint16_t) * config->x_pixels * config->y_pixels));
    memcpy(&intensities->config, config, sizeof(mb_config_t));

#ifdef MBBMP_THREADING
    thrd_t threads[processing_chunks];
    intensity_thread_workload_t workloads[processing_chunks];

    for (uint8_t i = 0; i < processing_chunks; ++i)
    {
        //Wait for our time to begin
        while (current_threads >= max_threads)
            thrd_yield();
        ++current_threads;//It's okay if we're not atomic with this comparison after the above since we only increment it here

        workloads[i].thread_num = i;
        workloads[i].intensities = intensities;
        thrd_create(&threads[i], generate_intensities_threaded, (void*)&workloads[i]);
    }

    for (uint8_t i = 0; i < processing_chunks; ++i)
    {
        thrd_join(threads[i], NULL);
    }
#else
    const double x_step = (config->max_x - config->min_x) / config->x_pixels;
    const double y_step = (config->max_y - config->min_y) / config->y_pixels;

    double x = config->min_x;

    //TODO use vectorization despite not having threading

    for (uint16_t i = 0; i < config->x_pixels; ++i)
    {
        double y = config->min_y;
        for (uint16_t j = 0; j < config->y_pixels; ++j)
        {
            intensities->intensities[i + (j * config->x_pixels)] = mandelbrot_iterations_basic(CMPLXF(x, y));
            y += y_step;
        }

        x += x_step;
    }
#endif

    return intensities;
}

void mb_destroy_intensities(mb_intensities_t* restrict intensities)
{
    free(intensities);
}

void mb_render_bw(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_1);
    bmp_palette_set_size(bitmap_to_init, 2);
    bmp_palette_colour_set(bitmap_to_init, 0, (palette_colour_t){.r = 0, .g = 0, .b = 0, .a = 0});
    bmp_palette_colour_set(bitmap_to_init, 1, (palette_colour_t){.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0});

    //TODO make this faster/multithreaded/vectorize
    for (uint16_t i = 0; i < intensities->config.x_pixels; ++i)
    {
        for (uint16_t j = 0; j < intensities->config.y_pixels; ++j)
        {
            uint32_t value = (intensities->intensities[i + (j * intensities->config.x_pixels)] == ITERATIONS) ? 0 : 1;

            bmp_px_set_1(bitmap_to_init, i, j, value);
        }
    }
}

void mb_render_grey_8(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_8);
    bmp_palette_set_size(bitmap_to_init, 256);

    //Simple pallete (maps index to brightness)
    for (uint16_t i = 0; i < 256; ++i)
        bmp_palette_colour_set(bitmap_to_init, i, (palette_colour_t) {.r = i, .g = i, .b = i, .a = 0});

    intensities_render_inverted_8(bitmap_to_init, intensities);
}

void mb_render_colour(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_24);

    //TODO make this faster/multithreaded/vectorize
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

            bmp_px_set_24(bitmap_to_init, i, j, value);
        }
    }
}

void mb_render_colour_8(const mb_intensities_t* restrict intensities, bmp_t* restrict bitmap_to_init)
{
    bmp_create(bitmap_to_init, intensities->config.x_pixels, intensities->config.y_pixels, BPP_8);
    bmp_palette_set_size(bitmap_to_init, 256);

    //Simple pallete (maps index to brightness)
    bmp_palette_colour_set(bitmap_to_init, 0, (palette_colour_t){.r = 0, .g = 0, .b = 0, .a = 0});//Special case: Non converge produces zero
    for (uint16_t i = 1; i < 256; ++i)
    {
        palette_colour_t colour = {.r = 0, .g = 0, .b = 0, .a = 0};

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

    intensities_render_inverted_8(bitmap_to_init, intensities);
}

/* Static Function Implementations */

static uint16_t mandelbrot_iterations_basic(complex double c)
{
    complex double z = 0;//z_0 = 0

    for (uint_fast16_t i = 0; i < ITERATIONS; ++i)
    {
        double real = creal(z);
        double imag = cimag(z);

        if (((real * real) + (imag * imag)) >= (CONVERGE_VALUE * CONVERGE_VALUE))//Check if abs(z) >= CONVERGE_VALUE
            return i;

        z = (z * z) + c;//z_(n+1) = z_n^2 + c
    }

    return ITERATIONS;//Failed to converge within ITERATIONS iterations
}

#ifdef __SSE2__
static __m128i mandelbrot_iterations_sse2_2(__m128d c_real, __m128d c_imag)
{
    const __m128d four = _mm_set_pd1(4.0);
    const __m128d two = _mm_set_pd1(2.0);
    const __m128i one_i = _mm_set1_epi64x(1);

    __m128i result = _mm_set1_epi64x(0);

    __m128d z_real = _mm_set_pd1(0);
    __m128d z_imag = _mm_set_pd1(0);
    for (uint_fast16_t i = 0; i < ITERATIONS; ++i)
    {
        //Calculate some values that are used below
        __m128d z_real_squared = _mm_mul_pd(z_real, z_real);
        __m128d z_imag_squared = _mm_mul_pd(z_imag, z_imag);

        //Check if the modulus of each z < the converge value of 2 (aka that they have converged)
        //We do this faster by doing (z_real * z_real) + (z_imag * z_imag) < (2 * 2)
        __m128d squared_sum = _mm_add_pd(z_real_squared, z_imag_squared);
        __m128i compare = _mm_castpd_si128(_mm_cmplt_pd(squared_sum, four));

        //If both complex numbers have converged (entire vector is 0), return early
        bool at_least_one_not_converged = (bool)_mm_movemask_epi8(compare);
        if (!at_least_one_not_converged)
            break;

        //Get next entries (For each complex number z, z_(n+1) = z_n^2 + c)
        __m128d temp_zreal = _mm_add_pd(_mm_sub_pd(z_real_squared, z_imag_squared), c_real);
        z_imag = _mm_add_pd(c_imag, _mm_mul_pd(two, _mm_mul_pd(z_real, z_imag)));
        z_real = temp_zreal;

        //Increment the corresponding count only if we haven't converged yet
        __m128i incrementor = _mm_and_si128(compare, one_i);//If a number hasn't converged, we will increment it's count
        result = _mm_add_epi64(result, incrementor);
    }

    //Pack the 64 bit values into the lower 32 bits (16 bits each)
    result = _mm_shuffle_epi32(result, _MM_SHUFFLE(0, 2, 0, 0));//TODO how does this work?
    return result;
}
#endif

#ifdef __AVX__
static __m256i mandelbrot_iterations_avx_4(__m256d c_real, __m256d c_imag)
{
    const __m256d four = _mm256_set1_pd(4.0);
    const __m256d two = _mm256_set1_pd(2.0);
    const __m256i one_i = _mm256_set1_epi64x(1);

    __m256i result = _mm256_set1_epi64x(0);

    __m256d z_real = _mm256_set1_pd(0);
    __m256d z_imag = _mm256_set1_pd(0);

    for (uint_fast16_t i = 0; i < ITERATIONS; ++i)
    {
        //Calculate some values that are used below
        __m256d z_real_squared = _mm256_mul_pd(z_real, z_real);
        __m256d z_imag_squared = _mm256_mul_pd(z_imag, z_imag);

        //Check if the modulus of each z < the converge value of 2 (aka that they have converged)
        //We do this faster by doing (z_real * z_real) + (z_imag * z_imag) < (2 * 2)
        __m256d squared_sum = _mm256_add_pd(z_real_squared, z_imag_squared);
        __m256d compare = _mm256_cmp_pd(squared_sum, four, _CMP_LT_OQ);

        //If both complex numbers have converged (entire vector is 0), return early
        bool at_least_one_not_converged = (bool)_mm256_movemask_pd(compare);
        if (!at_least_one_not_converged)
            break;

        //Get next entries (For each complex number z, z_(n+1) = z_n^2 + c)
        __m256d temp_zreal = _mm256_add_pd(_mm256_sub_pd(z_real_squared, z_imag_squared), c_real);
#ifdef __FMA__
        z_imag = _mm256_fmadd_pd(two, _mm256_mul_pd(z_real, z_imag), c_imag);
#else
        z_imag = _mm256_add_pd(_mm256_mul_pd(two, _mm256_mul_pd(z_real, z_imag)), c_imag);
#endif
        z_real = temp_zreal;

        //Increment the corresponding count only if we haven't converged yet
        __m256i incrementor = (__m256i)_mm256_and_pd(compare, (__m256d)one_i);//If a number hasn't converged, we will increment it's count

        //Actually perform the addition
#ifdef __AVX2__
        result = _mm256_add_epi64(result, incrementor);//Requires AVX2
#else//We must add the top and bottom seperatly
        __m128i lower_result = _mm256_castsi256_si128(result);
        __m128i upper_result = _mm256_extractf128_si256(result, 1);
        __m128i lower_incrementor = _mm256_castsi256_si128(incrementor);
        __m128i upper_incrementor = _mm256_extractf128_si256(incrementor, 1);
        __m128i lower_sum = _mm_add_epi64(lower_result, lower_incrementor);
        __m128i upper_sum = _mm_add_epi64(upper_result, upper_incrementor);
        result = _mm256_castsi128_si256(lower_sum);
        result = _mm256_insertf128_si256(result, upper_sum, 1);
#endif
    }

    //Pack the four 64 bit values into the lower 64 bits (16 bits each)
    //TODO can this be done faster
    //TODO can this be done faster when using avx2?
    //https://stackoverflow.com/questions/69408063/how-to-convert-int-64-to-int-32-with-avx-but-without-avx-512
    __m256 result_f = _mm256_castsi256_ps(result);
    __m128 lower_result_f = _mm256_castps256_ps128(result_f);
    __m128 upper_result_f = _mm256_extractf128_ps(result_f, 1);
    __m128 packed = _mm_shuffle_ps(lower_result_f, upper_result_f, _MM_SHUFFLE(2, 0, 2, 0));
    __m128i shuffle_mask = _mm_setr_epi8(0, 1, 4, 5, 8, 9, 12, 13, 0, 0, 0, 0, 0, 0, 0, 0);
    __m128i result_final = _mm_shuffle_epi8(_mm_castps_si128(packed), shuffle_mask);
    return _mm256_castsi128_si256(result_final);
}
#endif

static void intensities_render_inverted_8(bmp_t* restrict render, const mb_intensities_t* restrict intensities)//TODO make this faster/multithreaded/vectorize
{
#ifdef MBBMP_THREADING
    thrd_t threads[processing_chunks];
    render_thread_workload_t workloads[processing_chunks];

    for (uint8_t i = 0; i < processing_chunks; ++i)
    {
        //Wait for our time to begin
        while (current_threads >= max_threads)
            thrd_yield();
        ++current_threads;//It's okay if we're not atomic with this comparison after the above since we only increment it here

        workloads[i].thread_num = i;
        workloads[i].render = render;
        workloads[i].intensities = intensities;
        thrd_create(&threads[i], intensities_render_inverted_8_thread, (void*)&workloads[i]);
    }

    for (uint8_t i = 0; i < processing_chunks; ++i)
    {
        thrd_join(threads[i], NULL);
    }
#else
    for (uint16_t i = 0; i < intensities->config.x_pixels; ++i)
    {
        for (uint16_t j = 0; j < intensities->config.y_pixels; ++j)
        {
            uint32_t value;
            uint32_t intensity = intensities->intensities[i + (j * intensities->config.x_pixels)];

            if (intensity >= ITERATIONS)
                value = 0;
            else
                value = 255 - ((intensity * 255) / ITERATIONS);

            bmp_px_set_8(render, i, j, value);
        }
    }
#endif
}

#ifdef MBBMP_THREADING
static int intensities_render_inverted_8_thread(void* workload_)
{
    render_thread_workload_t* workload = (render_thread_workload_t*) workload_;

    const uint16_t thread_x_px = workload->intensities->config.x_pixels / processing_chunks;
    for (uint16_t i = thread_x_px * workload->thread_num; i < thread_x_px * (workload->thread_num + 1); ++i)
    {
        for (uint16_t j = 0; j < workload->intensities->config.y_pixels; ++j)
        {
            uint32_t value;
            uint32_t intensity = workload->intensities->intensities[i + (j * workload->intensities->config.x_pixels)];

            if (intensity >= ITERATIONS)
                value = 0;
            else
                value = 255 - ((intensity * 255) / ITERATIONS);

            bmp_px_set_8(workload->render, i, j, value);
        }
    }

    --current_threads;
    return 0;
}
#endif

#ifdef MBBMP_THREADING
static int generate_intensities_threaded(void* workload_)
{
    intensity_thread_workload_t* workload = (intensity_thread_workload_t*) workload_;

    mb_intensities_t* intensities = workload->intensities;
    mb_config_t* config = &intensities->config;

    const uint16_t thread_x_px = config->x_pixels / processing_chunks;//TODO what about rounding/excess pixels?

    /* This line here causes lots of issues.
     * When using low-precision numbers (floats), the subtraction done here causes a severe loss of precision that
     * leads to rendering glitches. While this can be mitigated by adding x_step to config->min_x until we reach the
     * desired spot, that is sort of a hack and is not efficient enough to be worth it.
     * We thus use doubles everywhere in mandelbrot_bmp_generator as they are a good balance of precision
     * (comparing with floats/long doubles) and can more easily be vectorized
    */
    const double thread_x_range = (config->max_x - config->min_x) / processing_chunks;

    const double x_step = (config->max_x - config->min_x) / config->x_pixels;
    const double y_step = (config->max_y - config->min_y) / config->y_pixels;

    double x = config->min_x + (thread_x_range * workload->thread_num);

#ifdef __AVX512F__
    assert(false);//TODO implement
#elif defined(__AVX__)
    //TODO what if not an number of pixels divisible by 4?
    for (uint16_t i = thread_x_px * workload->thread_num; i < thread_x_px * (workload->thread_num + 1); i += 4)
    {
        //Perform mandelbrot iterations on four values at once!
        __m256d real = _mm256_set_pd(x + (3 * x_step), x + (2 * x_step), x + x_step, x);

        double y = config->min_y;
        for (uint16_t j = 0; j < config->y_pixels; ++j)
        {
            //Perform mandelbrot iterations on four values at once!
            __m256d imag = _mm256_set1_pd(y);

            //The results are stored in the lower 64 bits (16 bits each) and so can be directly stored
            __m128i result = _mm256_extractf128_si256(mandelbrot_iterations_avx_4(real, imag), 0);
            _mm_storeu_si64(&intensities->intensities[i + (j * config->x_pixels)], result);

            y += y_step;
        }

        x += x_step * 4;
    }
#elif defined(__SSE2__)
    //TODO what if not an even number of pixels?
    for (uint16_t i = thread_x_px * workload->thread_num; i < thread_x_px * (workload->thread_num + 1); i += 2)
    {
        //Perform mandelbrot iterations on two values at once!
        __m128d real = _mm_set_pd(x + x_step, x);

        double y = config->min_y;
        for (uint16_t j = 0; j < config->y_pixels; ++j)
        {
            //Perform mandelbrot iterations on two values at once!
            __m128d imag = _mm_set_pd1(y);

            //The results are stored in the lower 32 bits (16 bits each) and so can be directly stored
            __m128i result = mandelbrot_iterations_sse2_2(real, imag);
            _mm_storeu_si32(&intensities->intensities[i + (j * config->x_pixels)], result);

            y += y_step;
        }

        x += x_step * 2;
    }
#else
    for (uint16_t i = thread_x_px * workload->thread_num; i < thread_x_px * (workload->thread_num + 1); ++i)
    {
        double y = config->min_y;

        for (uint16_t j = 0; j < config->y_pixels; ++j)
        {
            intensities->intensities[i + (j * config->x_pixels)] = mandelbrot_iterations_basic(CMPLXF(x, y));
            y += y_step;
        }

        x += x_step;
    }
#endif

    --current_threads;
    return 0;
}
#endif
