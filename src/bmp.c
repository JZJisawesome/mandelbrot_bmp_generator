/* Bitmap writing code, cut down for mandelbrot_bmp_generator
 * By: John Jekel
*/

/* Includes */

#include "bmp.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* Static Function Declarations */

static bool write_header_BITMAPINFOHEADER(const bmp_t* bmp, FILE* file, compression_t compression, size_t image_data_offset);
static void write_integer(FILE* file, uint32_t data, uint8_t num_lsbs);
static bool save_bi_rgb(const bmp_t* bmp, FILE* file);
static bool save_bi_rle8(const bmp_t* bmp, FILE* file);
static bool save_bi_rle4(const bmp_t* bmp, FILE* file);

/* Function Implementations */

//Creation/Destruction

void bmp_create(bmp_t* bmp, size_t width, size_t height, bpp_t bpp)
{
    assert(bmp);

    bmp->width = width;
    bmp->height = height;
    bmp->bpp = bpp;
    bmp->num_palette_colours = 0;

    size_t row_len_bits = bmp->width * bmp->bpp;
    bmp->row_len_bytes = row_len_bits / 8;
    if (row_len_bits % 8)//Leftover bits
        ++bmp->row_len_bytes;

    bmp->image_data_b = (uint8_t*) malloc(bmp->row_len_bytes * height);

    //Clear the bitmap if it is BPP_1 or BPP_4 so that the edge pixels are zeroed properly
    //TODO determine if this is actually necessary (certainly don't need to do all, just the right edges)
    //if (bpp == BPP_1 || (bpp == BPP_4))
    //    memset(bmp->image_data_b, 0, bmp->row_len_bytes * bmp->height);
}

void bmp_destroy(bmp_t* bmp)
{
    assert(bmp);

    if (bmp->num_palette_colours)
        free(bmp->palette);

    free(bmp->image_data_b);
}

//File saving

bool bmp_save(const bmp_t* bmp, const char* file_name, compression_t compression)
{
    assert(bmp);
    size_t image_data_offset = 54 + (bmp->num_palette_colours * 4);
    bool success = true;

    FILE* file = fopen(file_name, "w+b");
    if (!file)
        return false;//Failed to open file for writing+reading

    //Header
    if (!write_header_BITMAPINFOHEADER(bmp, file, compression, image_data_offset))
        success = false;

    //Colour table
    for (uint16_t i = 0; i < bmp->num_palette_colours; ++i)
    {
        //TODO error checking
        fputc(bmp->palette[i].b, file);
        fputc(bmp->palette[i].g, file);
        fputc(bmp->palette[i].r, file);
        fputc(0, file);//Reserved
    }
    //Pad rest of colour table with zeroes if it is BPP_8
    if (bmp->bpp == BPP_8)
        for (uint16_t i = 0; i < (256 - bmp->num_palette_colours); ++i)
            write_integer(file, 0, 4);//TODO error checking

    //Image data
    switch (compression)
    {
        case BI_RGB:
            if (!save_bi_rgb(bmp, file))
                success = false;
            break;
        case BI_RLE8:
            if (!save_bi_rle8(bmp, file))
                success = false;
            break;
        case BI_RLE4:
            if (!save_bi_rle4(bmp, file))
                success = false;
            break;
        default:
            success = false;
            break;
    }

    //Now handle file size
    size_t file_size = 0;
    rewind(file);

    while (fgetc(file) != EOF)
        ++file_size;

    //Write total file size
    fseek(file, 2, SEEK_SET);
    write_integer(file, file_size, 4);

    //Write size of (compressed) image data after decompression
    fseek(file, 34, SEEK_SET);
    write_integer(file, file_size - image_data_offset, 4);

    //Close file and return success
    fclose(file);
    if (!success)
    {
        remove(file_name);
        return false;
    }
    else
        return true;
}

//Pixel access

void bmp_px_set_1(bmp_t* bmp, size_t x, size_t y, bool value)
{
    //FIXME this is broken
    size_t index2 = (x / 8) + (y * bmp->row_len_bytes);
    uint8_t bit = 7 - (x % 8);

    bmp->image_data_b[index2] &= ~(1 << bit);
    bmp->image_data_b[index2] |= (value & 1) << bit;
}

void bmp_px_set_4(bmp_t* bmp, size_t x, size_t y, uint8_t value)
{
    size_t index1 = (x / 2) + (y * bmp->row_len_bytes);
    bool upper_nibble = !(x & 1);

    bmp->image_data_b[index1] &= upper_nibble ? 0x0F : 0xF0;
    bmp->image_data_b[index1] |= (value & 0xF) << (upper_nibble ? 4 : 0);
}

void bmp_px_set_8(bmp_t* bmp, size_t x, size_t y, uint8_t value)
{
    bmp->image_data_b[x + (y * bmp->width)] = value;
}

void bmp_px_set_16(bmp_t* bmp, size_t x, size_t y, uint16_t value)
{
    bmp->image_data_s[x + (y * bmp->width)] = value;//FIXME this depends on little-endianness
}

void bmp_px_set_24(bmp_t* bmp, size_t x, size_t y, uint32_t value)
{
    size_t index = (x * 3) + (y * (bmp->width * 3));
    bmp->image_data_b[index] = (value >> 16) & 0xFF;
    bmp->image_data_b[index + 1] = (value >> 8) & 0xFF;
    bmp->image_data_b[index + 2] = value & 0xFF;
}

//Palette manip
void bmp_palette_set_size(bmp_t* bmp, uint16_t num_palette_colours)
{
    assert(bmp);

    if (bmp->num_palette_colours)
    {
        if (!num_palette_colours)
            free(bmp->palette);
        else
            bmp->palette = (palette_colour_t*)realloc(bmp->palette, sizeof(palette_colour_t) * num_palette_colours);
    }
    else
        bmp->palette = (palette_colour_t*)malloc(sizeof(palette_colour_t) * num_palette_colours);

    bmp->num_palette_colours = num_palette_colours;
}

void bmp_palette_colour_set(bmp_t* bmp, uint16_t colour_num, palette_colour_t colour)
{
    assert(bmp);
    assert(colour_num < bmp->num_palette_colours);
    bmp->palette[colour_num] = colour;
}

/* Static Function Implementations */

static bool write_header_BITMAPINFOHEADER(const bmp_t* bmp, FILE* file, compression_t compression, size_t image_data_offset)
{
    fputs("BM", file);

    //Leave space for the end file size here
    write_integer(file, 0, 4);

    //Write 4 bytes of zeroes (reserved bytes)
    write_integer(file, 0, 4);

    //Write the offset into file of start of image data
    write_integer(file, image_data_offset, 4);

    //Write 40 as size of rest of the header (BITMAPINFOHEADER)
    write_integer(file, 40, 4);

    //Write width and height of image
    write_integer(file, bmp->width, 4);
    write_integer(file, bmp->height, 4);

    //Write 1 as the number of planes
    write_integer(file, 1, 2);

    //Write the bits per pixel
    write_integer(file, bmp->bpp, 2);

    //Write the compression type used and leave room for the compressed image data size to be written
    write_integer(file, compression, 4);
    write_integer(file, 0, 4);

    //Write the pixels per meter in the x and y directions (dummy values)
    write_integer(file, 1, 4);
    write_integer(file, 1, 4);

    //Palltte information
    write_integer(file, (bmp->bpp == BPP_8) ? 256 : bmp->num_palette_colours, 4);//If BPP_8, this is always 256
    write_integer(file, 0, 4);//All colours are important
    return true;//TODO proper error checking
}

static void write_integer(FILE* file, uint32_t data, uint8_t num_lsbs)
{
    //TODO error checking

    //BMP integers are stored little-endian (within the header)
    for (uint8_t i = 0; i < num_lsbs; ++i)
    {
        fputc(data & 0xFF, file);
        data >>= 8;
    }
}

static bool save_bi_rgb(const bmp_t* bmp, FILE* file)
{
    //Now we write the actual image data

    for (size_t i = 0; i < bmp->height; ++i)
    {
        //Write row bytes to file
        for (size_t j = 0; j < bmp->row_len_bytes; ++j)
            if (fputc(bmp->image_data_b[(i * bmp->row_len_bytes) + j], file) == EOF)
                return false;//If the byte couldn't be written, return false

        //Add padding 0 bytes if needed for 4 byte row allignment
        size_t remaining_alignment_bytes = (4 - (bmp->row_len_bytes % 4)) % 4;
        for (size_t j = 0; j < remaining_alignment_bytes; ++j)
            if (fputc(0, file) == EOF)
                return false;//If the byte couldn't be written, return false
    }

    return true;
}

static bool save_bi_rle8(const bmp_t* bmp, FILE* file)
{
    if (bmp->bpp != BPP_8)
        return false;

    //TODO improve compression ratios using absolute blocks in some places
    //At least on a per line basis (if no compression beats rle)

    for (size_t i = 0; i < bmp->height; ++i)
    {
        //RLE8 on a per-line basis
        uint8_t byte_count = 1;
        uint8_t replicate_byte = bmp->image_data_b[i * bmp->row_len_bytes];
        for (size_t j = byte_count; j < bmp->row_len_bytes; ++j)
        {
            bool match = bmp->image_data_b[j + (i * bmp->row_len_bytes)] == replicate_byte;

            if ((match && (byte_count == 255)) || !match)
            {
                fputc(byte_count, file);
                fputc(replicate_byte, file);

                byte_count = 1;
                replicate_byte = bmp->image_data_b[j + (i * bmp->row_len_bytes)];
            }
            else
                ++byte_count;
        }
        //We ended abruptly, so finish writing the current RLE8 chunk
        fputc(byte_count, file);
        fputc(replicate_byte, file);

        //Add special bytes for end of each row and of the bitmap
        fputc(0, file);

        if (i == (bmp->height - 1))
            fputc(1, file);
        else
            fputc(0, file);
    }

    return true;
}

static bool save_bi_rle4(const bmp_t* bmp, FILE* file)
{
    if (bmp->bpp != BPP_4)
        return false;

    //TODO be smarter about the fact that each pixel is only 4 bits
    //AKA accept scenarios where the two nibbles aren't equal
    //TODO improve compression ratios using absolute blocks in some places
    //At least on a per line basis (if no compression beats rle)

    for (size_t i = 0; i < bmp->height; ++i)
    {
        //RLE4 on a per-line basis
        uint8_t byte_count = 1;
        uint8_t pixel_count = 2;
        uint8_t replicate_byte = bmp->image_data_b[i * bmp->row_len_bytes];

        for (; byte_count < bmp->row_len_bytes; ++byte_count)
        {
            if ((replicate_byte & 0xF) == (replicate_byte >> 4))
                break;
            else
            {
                fputc(pixel_count, file);
                fputc(replicate_byte, file);
            }

            replicate_byte = bmp->image_data_b[byte_count + (i * bmp->row_len_bytes)];
        }

        for (size_t j = byte_count; j < bmp->row_len_bytes; ++j)
        {
            bool match = bmp->image_data_b[j + (i * bmp->row_len_bytes)] == replicate_byte;
            bool equal_nibbles = (replicate_byte & 0xF) == (replicate_byte >> 4);

            if ((match && (pixel_count >= 254)) || !match || !equal_nibbles)
            {
                fputc(pixel_count, file);
                fputc(replicate_byte, file);

                pixel_count = 2;
                replicate_byte = bmp->image_data_b[j + (i * bmp->row_len_bytes)];
            }
            else
                pixel_count += 2;
        }
        //We ended abruptly, so finish writing the current RLE4 chunk
        fputc(pixel_count, file);
        fputc(replicate_byte, file);

        //Add special bytes for end of each row and of the bitmap
        fputc(0, file);

        if (i == (bmp->height - 1))
            fputc(1, file);
        else
            fputc(0, file);
    }

    return true;
}
