/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * Copyright (c) 2019 Manfred Müller-Späth <fms1961@gmail.com>
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CS_MOS_LIBS_RGBLEDS_INCLUDE_MGOS_RGBLEDS_H_
#define CS_MOS_LIBS_RGBLEDS_INCLUDE_MGOS_RGBLEDS_H_

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "common/cs_dbg.h"
#include "mgos_bitbang.h"
#include "mgos_config.h"
#include "mgos_gpio.h"
#include "mgos_spi.h"
#include "mgos_system.h"
#include "mgos_timers.h"
#include "mgos_neopixel.h"
#include "bmpread.h"

static const uint8_t gammaR8[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5,
    5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9,
    9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14,
    15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
    23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33,
    33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45, 46,
    46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    62, 63, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 78, 79, 80,
    81, 83, 84, 85, 87, 88, 89, 91, 92, 94, 95, 97, 98, 99, 101, 102,
    104, 105, 107, 109, 110, 112, 113, 115, 116, 118, 120, 121, 123, 125, 127, 128,
    130, 132, 134, 135, 137, 139, 141, 143, 145, 146, 148, 150, 152, 154, 156, 158,
    160, 162, 164, 166, 168, 170, 172, 174, 177, 179, 181, 183, 185, 187, 190, 192,
    194, 196, 199, 201, 203, 206, 208, 210, 213, 215, 218, 220, 223, 225, 227, 230
};

static const uint8_t gammaG8[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

static const uint8_t gammaB8[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4,
    4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 8,
    8, 8, 8, 9, 9, 9, 10, 10, 10, 10, 11, 11, 12, 12, 12, 13,
    13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 19,
    20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 27, 27, 28, 28,
    29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
    40, 41, 42, 43, 44, 44, 45, 46, 47, 48, 49, 50, 51, 51, 52, 53,
    54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 69, 70,
    71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
    90, 92, 93, 94, 96, 97, 98, 100, 101, 103, 104, 106, 107, 109, 110, 112,
    113, 115, 116, 118, 119, 121, 122, 124, 126, 127, 129, 131, 132, 134, 136, 137,
    139, 141, 143, 144, 146, 148, 150, 152, 153, 155, 157, 159, 161, 163, 165, 167,
    169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 196, 198, 200
};

/* Callback to manipulate colors */
typedef void (*rgbleds_timer_callback)(void* param);

typedef enum {
    MGOS_RGBLEDS_TYPE_NEOPIXEL,
    MGOS_RGBLEDS_TYPE_WS2812,
    MGOS_RGBLEDS_TYPE_APA102,
    MGOS_RGBLEDS_TYPE_RGB_PWM,
    MGOS_RGBLEDS_TYPE_RGBW_PWM
} mgos_rgbleds_type;

/*
 * Pixel order: RGB, GRB, or BGR.
 */
typedef enum {
    MGOS_RGBLEDS_ORDER_RGB,
    MGOS_RGBLEDS_ORDER_GRB,
    MGOS_RGBLEDS_ORDER_BGR,
} mgos_rgbleds_order;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} mgos_rgbleds_coords;

typedef struct
{
    bool back;
    bool invert_colors;
    bool single_mode;
    bool toggle_odd;

    const char* color_file;

    double dim_all;
    double dim_blue;
    double dim_green;
    double dim_red;

    int animation;
    int color_count;
    int counter;
    int num_channels;
    int num_pixels;
    int panel_height;
    int panel_width;
    int pic_height;
    int pic_width;
    int pix_pos;
    int start;
    int rotate_col;
    int rotate_out;

    mgos_rgbleds_order color_order;
    mgos_rgbleds_type led_type;
    mgos_timer_id timer_id;
    rgbleds_timer_callback callback;

    size_t data_len;
    size_t timeout;

    uint8_t* color_shadows;
    uint8_t* color_values;
    uint8_t* data;
    uint8_t* lutB;
    uint8_t* lutG;
    uint8_t* lutR;

    void* driver;

} mgos_rgbleds;

static mgos_rgbleds* global_leds;

#if defined(__cplusplus)
extern "C" { // Make sure we have C-declarations in C++ programs
#endif

/*
 * Create and return a RGB LED strip object. Example:
 * ```c
 * struct mgos_rgbleds *mystrip = mgos_rgbleds_create(
 *     5, 16, MGOS_RGBLEDS_ORDER_GRB);
 * mgos_rgbleds_set(mystrip, 0, 12, 34, 56);
 * mgos_rgbleds_show(mystrip);
 *
 * mgos_rgbleds_clear(mystrip);
 * mgos_rgbleds_set(mystrip, 1, 12, 34, 56);
 * mgos_rgbleds_show(mystrip);
 * ```
 */
mgos_rgbleds* mgos_rgbleds_get_global();
mgos_rgbleds* mgos_rgbleds_create();
void mgos_rgbleds_get_colors(mgos_rgbleds* leds);
void mgos_rgbleds_adapt_colors(mgos_rgbleds* leds);
void mgos_rgbleds_adapt_viewport(mgos_rgbleds* leds, int vp_x, int vp_y, int col_x, int col_y, bool do_black);
void mgos_rgbleds_rotate_coords(int x, int y, int w, int h, int deg, mgos_rgbleds_coords *out);

/*
 * Set i-th pixel's RGB value. Each color (`r`, `g`, `b`) should be an integer
 * from 0 to 255; they are ints and not `uint8_t`s just for the FFI.
 *
 * Note that this only affects in-memory value of the pixel; you'll need to
 * call `mgos_rgbleds_show()` to apply changes.
 */
void mgos_rgbleds_set(mgos_rgbleds* leds, int pix, int r, int g, int b);

/*
 * Get i-th pixel's RGB value in CSV RGB format. Each color will be a string
 *  of integer from 0 to 255.
 * 
 * The result will be print in the 'out' buffer, the past length has 
 * to be at least 12 chars
 *
 * Note that this only reads in-memory value of the pixel.
 */
void mgos_rgbleds_get(mgos_rgbleds* leds, int i, char* out, int len);

/*
 * Clear in-memory values of the pixels.
 */
void mgos_rgbleds_clear(mgos_rgbleds* leds);

/*
 * Output values of the pixels.
 */
void mgos_rgbleds_show(mgos_rgbleds* leds);

void mgos_rgbleds_process(void* param);
void mgos_rgbleds_start(mgos_rgbleds* leds);
void mgos_rgbleds_stop(mgos_rgbleds* leds);
void mgos_rgbleds_transfer_gpio(mgos_rgbleds* leds);
void mgos_rgbleds_transfer_spi(mgos_rgbleds* leds);
void mgos_rgbleds_rotate_colors(mgos_rgbleds* leds);

/*
 * Free rgbleds instance.
 */
void mgos_rgbleds_free(mgos_rgbleds* leds);

#if defined(__cplusplus)
}
#endif

#endif /* CS_MOS_LIBS_RGBLEDS_INCLUDE_MGOS_RGBLEDS_H_ */
