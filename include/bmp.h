/* bmp.c
 * By: John Jekel
 *
 * Library for managing/saving bmp (BITMAPINFOHEADER-type) images.
 * Only "" types supported
 *
 * Create a bmp:
 * bmp_t foo;
 * bmp_create(&foo, width, height);
 *
 * TODO better documentation
 *
 *
 * ALWAYS DESTROY bmps AFTER LOADING/CREATING
 *
*/

#ifndef bmp_H
#define bmp_H

/* Includes */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Types */

typedef enum {BI_RGB = 0, BI_RLE8 = 1, BI_RLE4 = 2} compression_t;

typedef enum {BPP_1 = 1, BPP_4 = 4, BPP_8 = 8, BPP_16 = 16, BPP_24 = 24, BPP_32 = 32} bpp_t;

typedef struct
{
    uint8_t r, g, b;
} palette_colour_t;

typedef struct
{
    size_t width;//In pixels
    size_t height;//In pixels (number of rows)
    bpp_t bpp : 6;
    float width_m;//In meters
    float height_m;//In meters

    uint16_t num_palette_colours : 9;
    palette_colour_t* palette;

    size_t row_len_bytes;//Number of bytes per row

    union
    {
        uint8_t* image_data_b;
        uint16_t* image_data_s;
        uint32_t* image_data_w;
    };

} bmp_t;

/* Function/Class Declarations */

//Creation/Destruction
void bmp_create(bmp_t* bmp, size_t width, size_t height, bpp_t bpp);
void bmp_clear(bmp_t* bmp);//Zero entirety of bitmap
void bmp_destroy(bmp_t* bmp);

//File saving
bool bmp_save(const bmp_t* bmp, const char* file_name, compression_t compression);

//Palette manip
void bmp_palette_set_size(bmp_t* bmp, uint16_t num_palette_colours);
palette_colour_t bmp_palette_colour_get(const bmp_t* bmp, uint16_t colour_num);
void bmp_palette_colour_set(bmp_t* bmp, uint16_t colour_num, palette_colour_t colour);

//Pixel access
uint32_t bmp_px_get(const bmp_t* bmp, size_t x, size_t y);
void bmp_px_set(bmp_t* bmp, size_t x, size_t y, uint32_t value);
void bmp_px_set_rgb(bmp_t* bmp, size_t x, size_t y, uint8_t r, uint8_t g, uint8_t b);
void bmp_px_set_rgba(bmp_t* bmp, size_t x, size_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void bmp_px_set_1(bmp_t* bmp, size_t x, size_t y, bool value);
void bmp_px_set_4(bmp_t* bmp, size_t x, size_t y, uint8_t value);
void bmp_px_set_8(bmp_t* bmp, size_t x, size_t y, uint8_t value);
void bmp_px_set_16(bmp_t* bmp, size_t x, size_t y, uint16_t value);
void bmp_px_set_24(bmp_t* bmp, size_t x, size_t y, uint32_t value);
void bmp_px_set_32(bmp_t* bmp, size_t x, size_t y, uint32_t value);

//Other access
size_t bmp_width(const bmp_t* bmp);
size_t bmp_height(const bmp_t* bmp);
bpp_t bmp_bpp(const bmp_t* bmp);
void* bmp_data_ptr_get(bmp_t* bmp);
void bmp_dimension_set(bmp_t* bmp, float x_m, float y_m);//In meters

#endif//bmp_H
