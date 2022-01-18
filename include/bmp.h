/* Bitmap writing code, cut down for mandelbrot_bmp_generator
 * By: John Jekel
*/

#ifndef bmp_H
#define bmp_H

/* Includes */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Types */

typedef enum {BI_RGB = 0, BI_RLE8 = 1, BI_RLE4 = 2} compression_t;

typedef enum {BPP_1 = 1, BPP_4 = 4, BPP_8 = 8, BPP_16 = 16, BPP_24 = 24} bpp_t;

typedef struct
{
    uint8_t b, g, r, a;
} palette_colour_t;

typedef struct
{
    size_t width;//In pixels
    size_t height;//In pixels (number of rows)
    bpp_t bpp : 6;

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
void bmp_destroy(bmp_t* bmp);

//File saving
bool bmp_save(const bmp_t* bmp, const char* file_name, compression_t compression);

//Palette manip
void bmp_palette_set_size(bmp_t* bmp, uint16_t num_palette_colours);
void bmp_palette_colour_set(bmp_t* bmp, uint16_t colour_num, palette_colour_t colour);

//Pixel access
void bmp_px_set_1(bmp_t* bmp, size_t x, size_t y, bool value);
void bmp_px_set_4(bmp_t* bmp, size_t x, size_t y, uint8_t value);
void bmp_px_set_8(bmp_t* bmp, size_t x, size_t y, uint8_t value);
void bmp_px_set_16(bmp_t* bmp, size_t x, size_t y, uint16_t value);
void bmp_px_set_24(bmp_t* bmp, size_t x, size_t y, uint32_t value);

#endif//bmp_H
