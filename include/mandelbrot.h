/* Mandelbrot code
 * By: John Jekel
*/

#ifndef MANDELBROT_H//TODO
#define MANDELBROT_H//TODO

/* Includes */

#include <stdint.h>
#include "bmp.h"

/* Constants And Defines */

//TODO

/* Types */

typedef struct
{
    uint16_t x_pixels, y_pixels;
    long double min_x, max_x, min_y, max_y;

    //TODO colour stuffs here too

} mb_config_t;

typedef struct
{
    mb_config_t config;

    uint16_t intensities[];

} mb_intensities_t;

/* Function/Class Declarations */

void mb_set_total_active_threads(uint16_t threads);//TODO implement

//Dealing with intensities
mb_intensities_t* mb_generate_intensities(const mb_config_t* config);
void mb_destroy_intensities(mb_intensities_t* intensities);

//Dealing with rendering
void mb_render_bw(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);//1 bit bitmap
void mb_render_grey(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);//24 bit bitmap//TODO remove
void mb_render_grey_8(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);//8 bit bitmap
void mb_render_colour(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);//24 bit bitmap
void mb_render_colour_8(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);//8 bit bitmap

#endif//MANDELBROT_H//TODO
