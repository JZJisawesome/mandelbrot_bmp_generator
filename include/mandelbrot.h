/* NAME//TODO
 * By: John Jekel
 *
 * TODO description
 *
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
    float min_x, max_x, min_y, max_y;

    //TODO colour stuffs here too

} mb_config_t;

typedef struct
{
    mb_config_t config;

    uint16_t intensities[];

} mb_intensities_t;

/* Global Variables */

//TODO

/* Function/Class Declarations */

mb_intensities_t* mb_generate_intensities(const mb_config_t* config);
void mb_destroy_intensities(mb_intensities_t* intensities);

void mb_render_bw(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);
void mb_render_grey(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);
void mb_render_grey_8(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);
void mb_render_colour(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);//FIXME kind of ugly
void mb_render_colour_8(const mb_intensities_t* intensities, bmp_t* bitmap_to_init);

//void mb_config_init(mb_grid_t* intensity_grid, size_t x);

//void mb_grid_compute(mb_grid_t* intensity_grid);

#endif//MANDELBROT_H//TODO
