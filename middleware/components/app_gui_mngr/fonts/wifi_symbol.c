/*******************************************************************************
 * Size: 24 px
 * Bpp: 4
 * Opts: 
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef WIFI_SYMBOL
#define WIFI_SYMBOL 1
#endif

#if WIFI_SYMBOL

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0030 "0" */
    0x0, 0x0, 0x0, 0x0, 0x1, 0xbf, 0xfb, 0x10,
    0xc, 0xff, 0xff, 0xc0, 0x3f, 0xf5, 0x5f, 0xf4,
    0x5f, 0xe0, 0xe, 0xf5, 0x2f, 0xfc, 0xcf, 0xf2,
    0x7, 0xff, 0xff, 0x70, 0x0, 0x49, 0x94, 0x0,

    /* U+0031 "1" */
    0x0, 0x0, 0x0, 0x1, 0x47, 0x9a, 0xa9, 0x74,
    0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x6, 0xbf,
    0xff, 0xff, 0xff, 0xff, 0xfb, 0x60, 0x0, 0x0,
    0x0, 0x6, 0xef, 0xff, 0xfd, 0xa9, 0x9b, 0xcf,
    0xff, 0xfe, 0x60, 0x0, 0x1, 0xbf, 0xff, 0xa4,
    0x0, 0x0, 0x0, 0x0, 0x4a, 0xff, 0xfb, 0x10,
    0x3e, 0xff, 0xa1, 0x0, 0x0, 0x1, 0x10, 0x0,
    0x0, 0x1a, 0xff, 0xe3, 0xbf, 0xf5, 0x0, 0x4,
    0x9e, 0xff, 0xff, 0xe9, 0x40, 0x0, 0x5f, 0xfb,
    0xa, 0x30, 0x4, 0xdf, 0xff, 0xff, 0xff, 0xff,
    0xfc, 0x40, 0x3, 0xa0, 0x0, 0x0, 0x9f, 0xff,
    0xb7, 0x32, 0x23, 0x7b, 0xff, 0xf9, 0x0, 0x0,
    0x0, 0x8, 0xff, 0xb3, 0x0, 0x0, 0x0, 0x0,
    0x3b, 0xff, 0x80, 0x0, 0x0, 0x0, 0xb8, 0x0,
    0x2, 0x79, 0x97, 0x30, 0x0, 0x8b, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x3, 0xcf, 0xff, 0xff, 0xfc,
    0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4f,
    0xff, 0xda, 0xad, 0xff, 0xf4, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xb, 0xd3, 0x0, 0x0, 0x3d,
    0xb0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

    /* U+0032 "2" */
    0x0, 0x2, 0x79, 0x97, 0x30, 0x0, 0x3, 0xcf,
    0xff, 0xff, 0xfc, 0x30, 0x4f, 0xff, 0xda, 0xad,
    0xff, 0xf4, 0xb, 0xd3, 0x0, 0x0, 0x3d, 0xc0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
    0xbf, 0xfb, 0x10, 0x0, 0x0, 0xc, 0xff, 0xff,
    0xc0, 0x0, 0x0, 0x3f, 0xf5, 0x5f, 0xf4, 0x0,
    0x0, 0x5f, 0xe0, 0xe, 0xf5, 0x0, 0x0, 0x2f,
    0xfc, 0xcf, 0xf2, 0x0, 0x0, 0x7, 0xff, 0xff,
    0x70, 0x0, 0x0, 0x0, 0x49, 0x94, 0x0, 0x0,

    /* U+0033 "3" */
    0x0, 0x0, 0x0, 0x1, 0x47, 0x9a, 0xa9, 0x74,
    0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x6, 0xbf,
    0xff, 0xff, 0xff, 0xff, 0xfb, 0x60, 0x0, 0x0,
    0x0, 0x6, 0xef, 0xff, 0xfd, 0xa9, 0x9b, 0xcf,
    0xff, 0xfe, 0x60, 0x0, 0x1, 0xbf, 0xff, 0xa4,
    0x0, 0x0, 0x0, 0x0, 0x4a, 0xff, 0xfb, 0x10,
    0x3e, 0xff, 0xa1, 0x0, 0x0, 0x1, 0x10, 0x0,
    0x0, 0x1a, 0xff, 0xe3, 0xbf, 0xf5, 0x0, 0x4,
    0x9e, 0xff, 0xff, 0xe9, 0x40, 0x0, 0x5f, 0xfb,
    0xa, 0x30, 0x4, 0xdf, 0xff, 0xff, 0xff, 0xff,
    0xfc, 0x40, 0x3, 0xa0, 0x0, 0x0, 0x9f, 0xff,
    0xb7, 0x32, 0x23, 0x7b, 0xff, 0xf9, 0x0, 0x0,
    0x0, 0x8, 0xff, 0xb3, 0x0, 0x0, 0x0, 0x0,
    0x3b, 0xff, 0x80, 0x0, 0x0, 0x0, 0xb8, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x8b, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0,

    /* U+0034 "4" */
    0x0, 0x0, 0x0, 0x0, 0x11, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x49, 0xef, 0xff, 0xfe, 0x94,
    0x0, 0x0, 0x0, 0x4d, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xc4, 0x0, 0x9, 0xff, 0xfb, 0x73, 0x22,
    0x37, 0xbf, 0xff, 0x90, 0x8f, 0xfb, 0x30, 0x0,
    0x0, 0x0, 0x3, 0xbf, 0xf8, 0xb, 0x80, 0x0,
    0x27, 0x99, 0x73, 0x0, 0x8, 0xb0, 0x0, 0x0,
    0x3c, 0xff, 0xff, 0xff, 0xc3, 0x0, 0x0, 0x0,
    0x4, 0xff, 0xfd, 0xaa, 0xdf, 0xff, 0x40, 0x0,
    0x0, 0x0, 0xbd, 0x30, 0x0, 0x3, 0xdc, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x1b, 0xff, 0xb1,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xcf, 0xff,
    0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0xff,
    0x55, 0xff, 0x40, 0x0, 0x0, 0x0, 0x0, 0x5,
    0xfe, 0x0, 0xef, 0x50, 0x0, 0x0, 0x0, 0x0,
    0x2, 0xff, 0xcc, 0xff, 0x20, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x7f, 0xff, 0xf7, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x4, 0x99, 0x40, 0x0, 0x0,
    0x0,

    /* U+0035 "5" */
    0x0, 0x0, 0x0, 0x1, 0x47, 0x9a, 0xa9, 0x74,
    0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x6, 0xbf,
    0xff, 0xff, 0xff, 0xff, 0xfb, 0x60, 0x0, 0x0,
    0x0, 0x6, 0xef, 0xff, 0xfd, 0xa9, 0x9b, 0xcf,
    0xff, 0xfe, 0x60, 0x0, 0x1, 0xbf, 0xff, 0xa4,
    0x0, 0x0, 0x0, 0x0, 0x4a, 0xff, 0xfb, 0x10,
    0x3e, 0xff, 0xa1, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x1a, 0xff, 0xe3, 0xbf, 0xf5, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5f, 0xfb,
    0xa, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x3, 0xa0,

    /* U+0036 "6" */
    0x0, 0x0, 0x0, 0x1, 0x47, 0x9a, 0xa9, 0x74,
    0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x6, 0xbf,
    0xff, 0xff, 0xff, 0xff, 0xfb, 0x60, 0x0, 0x0,
    0x0, 0x6, 0xef, 0xff, 0xfd, 0xa9, 0x9b, 0xcf,
    0xff, 0xfe, 0x60, 0x0, 0x1, 0xbf, 0xff, 0xa4,
    0x0, 0x0, 0x0, 0x0, 0x4a, 0xff, 0xfb, 0x10,
    0x3e, 0xff, 0xa1, 0x0, 0x0, 0x1, 0x10, 0x0,
    0x0, 0x1a, 0xff, 0xe3, 0xbf, 0xf5, 0x0, 0x4,
    0x9e, 0xff, 0xff, 0xe9, 0x40, 0x0, 0x5f, 0xfb,
    0xa, 0x30, 0x4, 0xdf, 0xff, 0xff, 0xff, 0xff,
    0xfc, 0x40, 0x3, 0xa0, 0x0, 0x0, 0x9f, 0xff,
    0xb7, 0x32, 0x23, 0x7b, 0xff, 0xf9, 0x0, 0x0,
    0x0, 0x8, 0xff, 0xb3, 0x0, 0x0, 0x0, 0x0,
    0x3b, 0xff, 0x80, 0x0, 0x0, 0x0, 0xb8, 0x0,
    0x2, 0x79, 0x97, 0x30, 0x0, 0x8b, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x3, 0xcf, 0xff, 0xff, 0xfc,
    0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4f,
    0xff, 0xda, 0xad, 0xff, 0xf4, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xb, 0xd3, 0x0, 0x0, 0x3d,
    0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x1, 0xbf, 0xfb, 0x10,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xc, 0xff, 0xff, 0xc0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x3f, 0xf5, 0x5f, 0xf4,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x5f, 0xe0, 0xe, 0xf5, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x2f, 0xfc, 0xcf, 0xf2,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x7, 0xff, 0xff, 0x70, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x49, 0x94, 0x0,
    0x0, 0x0, 0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 384, .box_w = 8, .box_h = 8, .ofs_x = 8, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 384, .box_w = 24, .box_h = 14, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 200, .adv_w = 384, .box_w = 12, .box_h = 12, .ofs_x = 6, .ofs_y = 0},
    {.bitmap_index = 272, .adv_w = 384, .box_w = 24, .box_h = 11, .ofs_x = 0, .ofs_y = 10},
    {.bitmap_index = 404, .adv_w = 384, .box_w = 18, .box_h = 17, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 557, .adv_w = 384, .box_w = 24, .box_h = 7, .ofs_x = 0, .ofs_y = 14},
    {.bitmap_index = 641, .adv_w = 384, .box_w = 24, .box_h = 21, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 48, .range_length = 7, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t wifi_symbol = {
#else
lv_font_t wifi_symbol = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 21,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if WIFI_SYMBOL*/

