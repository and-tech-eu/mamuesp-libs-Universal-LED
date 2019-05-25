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

#ifndef __MGOS_UNIVERSAL_LED_H_
#define __MGOS_UNIVERSAL_LED_H_

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
#include "mgos_bmp_loader.h"
#include "ws2812_control.h"
#include "mgos_common_tools.h"

typedef enum {
    MGOS_RGBLEDS_ACT_INIT,
    MGOS_RGBLEDS_ACT_EXIT,
    MGOS_RGBLEDS_ACT_LOOP
} mgos_rgbleds_action;

/* Callback to manipulate colors */
typedef void (*mgos_rgbleds_timer_callback)(void* param, mgos_rgbleds_action action);

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
    int pix_pos;
} mgos_rgbleds_coords;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} mgos_rgbleds_rgb;

typedef struct
{
    bool back;
    bool invert_colors;
    bool single_mode;
    bool soft_dim;
    bool finish;
    bool fit_horz;
    bool fit_vert;

    const char* color_file;
    const char* animation;

    double dim_all;
    double gamma_red;
    double gamma_green;
    double gamma_blue;

    int* gaps;
    int gaps_len;
    int counter;
    int num_channels;
    int pic_channels;
    int panel_height;
    int panel_width;
    int pic_height;
    int pic_width;
    int pix_pos;
    int internal_loops;
    int start;
    int rotate_col;
    int rotate_out;

    mgos_rgbleds_order color_order;
    mgos_rgbleds_type led_type;
    mgos_timer_id timer_id;
    mgos_rgbleds_timer_callback callback;

    size_t timeout;

    uint8_t* color_values;
    uint8_t* data;
    uint8_t* lutB;
    uint8_t* lutG;
    uint8_t* lutR;
    
    void *audio_data;
    void* driver;
} mgos_rgbleds;

mgos_rgbleds* global_leds;

#if defined(__cplusplus)
extern "C" { // Make sure we have C-declarations in C++ programs
#endif

mgos_rgbleds* mgos_universal_led_get_global();
mgos_rgbleds* mgos_universal_led_create();

void mgos_universal_led_get_gaps(mgos_rgbleds* leds);
bool mgos_universal_led_is_in_gaps(mgos_rgbleds* leds, int pix);

void mgos_universal_led_set_orientation(mgos_rgbleds* leds);
void mgos_universal_led_get_bitmap(mgos_rgbleds* leds);
void mgos_universal_led_rotate_coords(int x, int y, int w, int h, int deg, mgos_rgbleds_coords* out);
void mgos_universal_led_prepare_gamma(mgos_rgbleds* leds);
void mgos_universal_led_scale_bitmap(mgos_rgbleds* leds);

tools_rgb_data mgos_universal_led_lookup_gamma(mgos_rgbleds* leds, tools_rgb_data pix);

void mgos_universal_led_plot_pixel(mgos_rgbleds* leds, int x, int y, tools_rgb_data color, bool invert_toggle_odd);
void mgos_universal_led_plot_all(mgos_rgbleds* leds, tools_rgb_data pix_col);
void mgos_universal_led_line(mgos_rgbleds* leds, int x1, int y1, int x2, int y2, tools_rgb_data color);
void mgos_universal_led_ellipse(mgos_rgbleds* leds, int origin_x, int origin_y, int width, int height, tools_rgb_data color);

/*
 * Set i-th pixel's RGB value. Each color (`r`, `g`, `b`) should be an integer
 * from 0 to 255; they are ints and not `uint8_t`s just for the FFI.
 *
 * Note that this only affects in-memory value of the pixel; you'll need to
 * call `mgos_universal_led_show()` to apply changes.
 */
void mgos_universal_led_set(mgos_rgbleds* leds, int pix, int r, int g, int b);
void mgos_universal_led_set_pixel(mgos_rgbleds* leds, int pix, tools_rgb_data color);
void mgos_universal_led_set_all(mgos_rgbleds* leds, tools_rgb_data pix_col);
void mgos_universal_led_set_from_buffer(mgos_rgbleds* leds, int x_offset, int y_offset, bool wrap);
void mgos_universal_led_set_adapted(mgos_rgbleds* leds, int vp_x, int vp_y, int col_x, int col_y, bool do_black);
void mgos_universal_led_unset(mgos_rgbleds* leds, int pix);

/*
 * Get i-th pixel's RGB value in CSV RGB format. Each color will be a string
 *  of integer from 0 to 255.
 * 
 * The result will be print in the 'out' buffer, the past length has 
 * to be at least 12 chars
 *
 * Note that this only reads in-memory value of the pixel.
 */
tools_rgb_data mgos_universal_led_get(mgos_rgbleds* leds, int i, char* out, int len);
tools_rgb_data mgos_universal_led_get_from_pos(mgos_rgbleds* leds, int x, int y, char* out, int len);

/*
 * Clear in-memory values of the pixels.
 */
void mgos_universal_led_clear(mgos_rgbleds* leds);

/*
 * Output values of the pixels.
 */
void mgos_universal_led_show(mgos_rgbleds* leds);

void mgos_universal_led_process(void* param);
void mgos_universal_led_one_shot(mgos_rgbleds* leds);
void mgos_universal_led_start_delayed(mgos_rgbleds* leds);
void mgos_universal_led_start(mgos_rgbleds* leds);
void mgos_universal_led_stop(mgos_rgbleds* leds);
void mgos_universal_led_transfer_gpio(mgos_rgbleds* leds);
void mgos_universal_led_transfer_spi(mgos_rgbleds* leds);
void mgos_universal_led_rotate_bitmap(mgos_rgbleds* leds);

void mgos_universal_led_resume_delayed(mgos_rgbleds* leds);
void mgos_universal_led_resume(mgos_rgbleds* leds);
void mgos_universal_led_pause(mgos_rgbleds* leds);


/*
 * Free universal_led instance.
 */
void mgos_universal_led_free(mgos_rgbleds* leds);
void mgos_universal_led_log_data(mgos_rgbleds* leds, enum cs_log_level level, const char* name);
void mgos_universal_led_setup(mgos_rgbleds* leds);

#if defined(__cplusplus)
}
#endif

#endif /* __MGOS_UNIVERSAL_LED_H_ */
