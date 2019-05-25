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

#include "mgos_universal_led.h"
#include "mgos_common_tools.h"

/*
#include "debug_colors.h"

static int rgb_to_ansi(int red, int grn, int blu)
{
    uint32_t d0 = red * red + grn * grn + blu * blu;
    int colIdx = 0;
    for (int cl = 0; cl < 256; cl++) {
        int p_red = colorPalette[cl][0];
        int p_grn = colorPalette[cl][1];
        int p_blu = colorPalette[cl][2];
        int d = (red - p_red) * (red - p_red) + (grn - p_grn) * (grn - p_grn) + (blu - p_blu) * (blu - p_blu);
        if (d0 >= d) {
            colIdx = cl;
            break;
        }
    }
    return colIdx;
}
*/

mgos_rgbleds* mgos_universal_led_get_global()
{
    if (global_leds == NULL) {
        global_leds = mgos_universal_led_create();
    }
    return global_leds;
}

mgos_rgbleds* mgos_universal_led_create()
{
    mgos_rgbleds* leds = calloc(1, sizeof(mgos_rgbleds));
    int pin = mgos_sys_config_get_universal_led_pin();

    leds->color_file = (char*)mgos_sys_config_get_universal_led_color_file();
    leds->gamma_red = mgos_sys_config_get_universal_led_gamma_red();
    leds->gamma_green = mgos_sys_config_get_universal_led_gamma_green();
    leds->gamma_blue = mgos_sys_config_get_universal_led_gamma_blue();
    leds->color_order = mgos_sys_config_get_universal_led_color_order();
    leds->rotate_col = mgos_sys_config_get_universal_led_rotate_col();
    leds->rotate_out = mgos_sys_config_get_universal_led_rotate_out();
    leds->num_channels = mgos_sys_config_get_universal_led_num_channels();
    leds->pic_channels = mgos_sys_config_get_universal_led_pic_channels();
    leds->panel_width = mgos_sys_config_get_universal_led_panel_width();
    leds->panel_height = mgos_sys_config_get_universal_led_panel_height();
    leds->dim_all = mgos_sys_config_get_universal_led_dim_all();
    leds->soft_dim = mgos_sys_config_get_universal_led_soft_dim();
    leds->led_type = mgos_sys_config_get_universal_led_led_type();
    leds->single_mode = mgos_sys_config_get_universal_led_single_mode();
    leds->invert_colors = mgos_sys_config_get_universal_led_invert_colors();
    leds->timeout = mgos_sys_config_get_universal_led_timeout();

    mgos_universal_led_set_orientation(leds);
    mgos_universal_led_get_gaps(leds);
    mgos_universal_led_prepare_gamma(leds);

    uint32_t panel_data_len = leds->panel_width * leds->panel_height * leds->num_channels;

    leds->color_values = calloc(1, panel_data_len);
    memset(leds->color_values, 0x0f, panel_data_len);

    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_WS2812:
        ws2812_control_init(pin, 0, leds->panel_width * leds->panel_height);
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_APA102:
        leds->driver = mgos_neopixel_create_channeled(pin, leds->panel_width * leds->panel_height, leds->color_order, leds->num_channels);
        leds->data = ((struct mgos_neopixel*)leds->driver)->data;
        break;
    default:
        break;
    }

    mgos_universal_led_clear(leds);
    mgos_universal_led_show(leds);
    return leds;
}

void mgos_universal_led_prepare_gamma(mgos_rgbleds* leds)
{
    double gamma[3];
    uint8_t* luts[3];

    gamma[0] = leds->gamma_red;
    gamma[1] = leds->gamma_green;
    gamma[2] = leds->gamma_blue;

    for (int col = 0; col < 3; col++) {
        if (gamma[col] != 1.0) {
            luts[col] = calloc(1, 256);
            for (int i = 0; i < 256; i++) {
                double value = i / 255.0;
                double gammaExp = (1.0 * gamma[col]);
                luts[col][i] = (uint8_t)round(255.0 * pow(value, gammaExp));
                LOG(LL_DEBUG, ("Gamma table %s: gamma %.02f, in: %d, out: %d", (col == 0 ? "red" : col == 1 ? "green" : "blue"), gamma[col], i, luts[col][i]));
            }
        }
    }
    leds->lutR = luts[0];
    leds->lutG = luts[1];
    leds->lutB = luts[2];
}

void mgos_universal_led_get_gaps(mgos_rgbleds* leds)
{
    char** str_gaps;
    leds->gaps_len = tools_str_split(mgos_sys_config_get_universal_led_gaps(), ',', &str_gaps);
    if (leds->gaps_len > 0) {
        leds->gaps = calloc(leds->gaps_len, sizeof(uint32_t));
        for (int i = 0; i < leds->gaps_len; i++) {
            leds->gaps[i] = atoi(str_gaps[i]);
        }
        tools_str_split_free(str_gaps, leds->gaps_len);
    }
}

bool mgos_universal_led_is_in_gaps(mgos_rgbleds* leds, int pix)
{
    bool result = false;
    if (leds->gaps_len > 0)
        for (int i = 0; i < leds->gaps_len; i++) {
            if (pix == (int)leds->gaps[i]) {
                result = true;
                break;
            }
        }
    return result;
}

void mgos_universal_led_get_bitmap(mgos_rgbleds* leds)
{
    if (leds->color_file != NULL) {
        bmpread_t* p_bmp_out = mgos_bmp_loader_create();
        if (mgos_bmp_loader_load(p_bmp_out, leds->color_file, BMPREAD_TOP_DOWN | BMPREAD_BYTE_ALIGN | BMPREAD_ANY_SIZE)) {
            leds->pic_width = p_bmp_out->width;
            leds->pic_height = p_bmp_out->height;
            uint32_t pic_data_len = leds->pic_width * leds->pic_height * 3;
            LOG(LL_DEBUG, ("Bitmap file width: <%d>, height: <%d>, data: <%d>", leds->pic_width, leds->pic_height, pic_data_len));
            if (leds->pic_width > leds->panel_width || leds->pic_height > leds->panel_height) {
                leds->color_values = realloc(leds->color_values, pic_data_len);
            }
            memcpy(leds->color_values, p_bmp_out->data, pic_data_len);
            mgos_bmp_loader_free(p_bmp_out);
            LOG(LL_DEBUG, ("Bitmap file <%s> loaded and processed!", leds->color_file));
        } else {
            LOG(LL_ERROR, ("Bitmap file <%s> could not be loaded!", leds->color_file));
        }
    } else {
        LOG(LL_ERROR, ("Bitmap file name is NULL, so colors could not be loaded!"));
    }
}

void mgos_universal_led_scale_bitmap(mgos_rgbleds* leds)
{
    if (leds->pic_width == 0 || leds->pic_height == 0) {
        LOG(LL_ERROR, ("Bitmap %s is empty!", leds->color_file));
        return;
    }

    double scale_y = leds->fit_vert ? (leds->panel_height * 1.0) / (leds->pic_height * 1.0) : 1.0;
    double scale_x = leds->fit_horz ? (leds->panel_width * 1.0) / (leds->pic_width * 1.0) : 1.0;
    if (leds->fit_vert && !leds->fit_horz) {
        scale_x = scale_y;
    } else if (leds->fit_horz && !leds->fit_vert) {
        scale_y = scale_x;
    }
    int new_width = (int)round(scale_x * leds->pic_width);
    int new_height = (int)round(scale_y * leds->pic_height);
    LOG(LL_INFO, ("Scale: w: %d, h: %d, x: %f, y: %f, nw: %d, nh: %d", leds->pic_width, leds->pic_height, scale_x, scale_y, new_width, new_height));

    mgos_rgbleds* target = calloc(new_width * new_height, sizeof(mgos_rgbleds));
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            int x_src = (int)round(x / scale_x);
            x_src = (x_src < 0) ? 0 : (x_src >= leds->pic_width) ? (leds->pic_width - 1) : x_src;
            int y_src = (int)round(y / scale_y);
            y_src = (y_src < 0) ? 0 : (y_src >= leds->pic_height) ? (leds->pic_height - 1) : y_src;
            mgos_universal_led_rgb* p_tgt = ((mgos_universal_led_rgb*)target) + ((y * new_width) + x);
            mgos_universal_led_rgb* p_src = ((mgos_universal_led_rgb*)leds->color_values) + ((y_src * leds->pic_width) + x_src);
            *p_tgt = *p_src;
            LOG(LL_DEBUG, ("Src - x: %d, y: %d, Tgt - x: %d, y: %d", x_src, y_src, x, y));
        }
    }

    free(leds->color_values);
    leds->color_values = (uint8_t*)target;
    leds->pic_width = new_width;
    leds->pic_height = new_height;
}

void mgos_universal_led_set_orientation(mgos_rgbleds* leds)
{
    int deg = mgos_sys_config_get_universal_led_rotate_out();
    bool do_swap = (deg % 90) == 0 && ((deg / 90) & 0x1) == 1;
    if (do_swap) {
        mgos_rgbleds_coords dst;
        int width = leds->panel_height;
        int height = leds->panel_width;
        leds->panel_height = height;
        leds->panel_width = width;
    }
}

void mgos_universal_led_rotate_bitmap(mgos_rgbleds* leds)
{
    uint32_t pic_data_len = leds->pic_width * leds->pic_height * 3;
    uint8_t* dest = calloc(pic_data_len, 1);
    int new_width = leds->pic_height;
    int new_height = leds->pic_width;
    for (int y = 0; y < leds->pic_height; y++) {
        for (int x = 0; x < leds->pic_width; x++) {
            int y_dest = x;
            int x_dest = leds->pic_height - y - 1;
            uint8_t* p_src = (((y * leds->pic_width) + x) * leds->num_channels) + leds->color_values;
            uint8_t* p_dest = (((y_dest * new_width) + x_dest) * leds->num_channels) + dest;
            memcpy(p_dest, p_src, leds->num_channels);
        }
    }
    free(leds->color_values);
    leds->color_values = dest;
    leds->pic_width = new_height;
    leds->pic_height = new_width;
}

tools_rgb_data mgos_universal_led_lookup_gamma(mgos_rgbleds* leds, tools_rgb_data pix)
{
    double dim_all = (leds->soft_dim || (leds->led_type != MGOS_RGBLEDS_TYPE_APA102)) ? leds->dim_all : 1.0;
    uint8_t cols[3];
    uint8_t* luts[3];

    cols[0] = pix.r;
    cols[1] = pix.g;
    cols[2] = pix.b;

    luts[0] = leds->lutR;
    luts[1] = leds->lutG;
    luts[2] = leds->lutB;

    for (int i = 0; i < 3; i++) {
        int val = leds->invert_colors ? (255 - cols[i]) : cols[i];
        if (luts[i] != NULL) {
            val = (int)luts[i][val];
        }

        val = (val > 255) ? 255 : val;
        val = (val < 0) ? 0 : val;
        cols[i] = val;
    }

    pix.r = cols[0];
    pix.g = cols[1];
    pix.b = cols[2];

    if ((int)dim_all != 1) {
        double h, s, v;
        tools_rgb_to_hsv(pix, &h, &s, &v);
        v = v * dim_all;
        pix = tools_hsv_to_rgb(h, s, v);
    }

    return pix;
}

void mgos_universal_led_set_adapted(mgos_rgbleds* leds, int vp_x, int vp_y, int col_x, int col_y, bool do_black)
{
    uint32_t pic_data_len = leds->pic_width * leds->pic_height * leds->pic_channels;
    mgos_rgbleds_coords col;
    mgos_universal_led_rotate_coords(col_x, col_y, leds->pic_width, leds->pic_height, leds->rotate_col, &col);
    int col_runner = ((col.y * col.w) + (col.x % col.w)) % pic_data_len;
    uint8_t black[4] = { 0, 0, 0, 0 };
    uint8_t* p_pix = do_black ? black : leds->color_values + (col_runner * leds->num_channels);

    mgos_rgbleds_coords dst;
    mgos_universal_led_rotate_coords(vp_x, vp_y, leds->panel_width, leds->panel_height, leds->rotate_out, &dst);
    bool isOdd = ((dst.y & 0x01) == 1);
    bool toggle_odd = mgos_sys_config_get_universal_led_toggle_odd();
    int corr_x = (toggle_odd && isOdd) ? (dst.w - (dst.x + 1)) : dst.x;
    vp_x = corr_x;

    tools_rgb_data out_pix;
    out_pix.r = p_pix[0];
    out_pix.g = p_pix[1];
    out_pix.b = p_pix[2];
    mgos_universal_led_plot_pixel(leds, vp_x, vp_y, out_pix, false);
}

void mgos_universal_led_rotate_coords(int x, int y, int w, int h, int deg, mgos_rgbleds_coords* out)
{
    out->w = w;
    out->h = h;
    switch (deg) {
    case 90:
        out->x = y;
        out->y = (w - x - 1) % w;
        out->w = h;
        out->h = w;
        break;
    case 180:
        out->x = x;
        out->y = (h - y - 1) % h;
        break;
    case 270:
        out->x = (h - y - 1) % h;
        out->y = (w - x - 1) % w;
        out->w = h;
        out->h = w;
        break;
    default:
        out->x = x;
        out->y = y;
        break;
    }
    out->pix_pos = out->x + (out->y * out->w);
}

static int mgos_universal_led_calc_pix_num(mgos_rgbleds* leds, int x, int y, bool invert_toggle_odd)
{
    uint16_t led_pos = 0;
    bool isOdd;

    if (x < 0 || y < 0 || x >= leds->panel_width || y >= leds->panel_height) {
        // do nothing if out of bounds
        return -1;
    }

    bool is_updown = mgos_sys_config_get_universal_led_updown();
    bool toggle_odd = mgos_sys_config_get_universal_led_toggle_odd();
    toggle_odd = invert_toggle_odd ? ~toggle_odd : toggle_odd; 
    const char* alignement = mgos_sys_config_get_universal_led_alignement();
    char align = 'N';
    if (alignement != NULL && strlen(alignement) > 0) {
        align = alignement[0];
    }

    switch (align) {
    case 'Y':
        isOdd = ((x & 0x01) == 1);
        y = is_updown ? leds->panel_height - 1 - y : y;
        y = (toggle_odd && isOdd) ? (leds->panel_height - (y + 1)) : y;
        led_pos = ((x * leds->panel_height)) + y;
        break;
    default:
    case 'X':
        isOdd = ((y & 0x01) == 1);
        x = is_updown ? leds->panel_width - 1 - x : x;
        x = (toggle_odd && isOdd) ? (leds->panel_width - (x + 1)) : x;
        led_pos = ((y * leds->panel_width) + x);
        break;
    }

    return led_pos;
}

void mgos_universal_led_plot_pixel(mgos_rgbleds* leds, int x, int y, tools_rgb_data color, bool invert_toggle_odd)
{
    int led_pos = mgos_universal_led_calc_pix_num(leds, x, y, invert_toggle_odd);
    uint32_t num_pix = leds->panel_width * leds->panel_height;

    if (led_pos >= 0 && led_pos < num_pix) {
        color = mgos_universal_led_lookup_gamma(leds, color);
        mgos_universal_led_set_pixel(leds, led_pos, color);
    }
}

void mgos_universal_led_plot_all(mgos_rgbleds* leds, tools_rgb_data pix_col)
{
    for (int x = 0; x < leds->panel_width; x++) {
        for (int y = 0; y < leds->panel_height; y++) {
            mgos_universal_led_plot_pixel(leds, x, y, pix_col, false);
        }
    }
}

void mgos_universal_led_line(mgos_rgbleds* leds, int x1, int y1, int x2, int y2, tools_rgb_data color)
{
    // Bresenham for lines
    int x, y, dx, dy, error, a, b, x_step, y_step;

    dx = x2 - x1;
    dy = y2 - y1;
    x_step = 1;
    y_step = 1;
    x = x1;
    y = y1;
    if (dx < 0) {
        dx = -dx;
        x_step = -1;
    }
    if (dy < 0) {
        dy = -dy;
        y_step = -1;
    }
    a = dx + dx;
    b = dy + dy;
    if (dy <= dx) {
        error = -dx;
        while (x != x2) {
            mgos_universal_led_plot_pixel(leds, x, y, color, false);
            error = error + b;
            if (error > 0) {
                y = y + y_step;
                error = error - a;
            }
            x = x + x_step;
        } // while
    } else {
        error = -dy;
        while (y != y2) {
            mgos_universal_led_plot_pixel(leds, x, y, color, false);
            error = error + a;
            if (error > 0) {
                x = x + x_step;
                error = error - b;
            }
            y = y + y_step;
        } // while
    }
    mgos_universal_led_plot_pixel(leds, x, y, color, false);
}

void mgos_universal_led_ellipse(mgos_rgbleds* leds, int origin_x, int origin_y, int width, int height, tools_rgb_data color)
{

    int hh = height * height;
    int ww = width * width;
    int hhww = hh * ww;
    int x0 = width;
    int dx = 0;

    // do the horizontal diameter
    for (int x = -width; x <= width; x++) {
        mgos_universal_led_plot_pixel(leds, origin_x + x, origin_y, color, false);
    }

    // now do both halves at the same time, away from the diameter
    for (int y = 1; y <= height; y++) {
        int x1 = x0 - (dx - 1); // try slopes of dx - 1 or more
        for (; x1 > 0; x1--) {
            if (x1 * x1 * hh + y * y * ww <= hhww) {
                break;
            }
        }

        dx = x0 - x1; // current approximation of the slope
        x0 = x1;

        for (int x = -x0; x <= x0; x++) {
            mgos_universal_led_plot_pixel(leds, origin_x + x, origin_y - y, color, false);
            mgos_universal_led_plot_pixel(leds, origin_x + x, origin_y + y, color, false);
        }
    }
}

void mgos_universal_led_set(mgos_rgbleds* leds, int pix, int r, int g, int b)
{
    if (mgos_universal_led_is_in_gaps(leds, pix) == false) {
        switch (leds->led_type) {
        case MGOS_RGBLEDS_TYPE_NEOPIXEL:
        case MGOS_RGBLEDS_TYPE_WS2812:
        case MGOS_RGBLEDS_TYPE_APA102:
            mgos_neopixel_set((struct mgos_neopixel*)leds->driver, pix, r, g, b);
            break;
        default:
            break;
        }
    } else {
        mgos_universal_led_unset(leds, pix);
    }
}

void mgos_universal_led_set_pixel(mgos_rgbleds* leds, int pix, tools_rgb_data color)
{
    mgos_universal_led_set(leds, pix, color.r, color.g, color.b);
}

void mgos_universal_led_set_all(mgos_rgbleds* leds, tools_rgb_data pix_col)
{
    uint32_t num_pixels = leds->panel_width * leds->panel_height;
    for (int pix = 0; pix < num_pixels; pix++) {
        if (mgos_universal_led_is_in_gaps(leds, pix) == false) {
            switch (leds->led_type) {
            case MGOS_RGBLEDS_TYPE_NEOPIXEL:
            case MGOS_RGBLEDS_TYPE_WS2812:
            case MGOS_RGBLEDS_TYPE_APA102:
                mgos_neopixel_set((struct mgos_neopixel*)leds->driver, pix, pix_col.r, pix_col.g, pix_col.b);
                break;
            default:
                break;
            }
        } else {
            mgos_universal_led_unset(leds, pix);
        }
    }
}

void mgos_universal_led_set_from_buffer(mgos_rgbleds* leds, int x_offset, int y_offset, bool wrap)
{
    uint32_t num_rows = leds->panel_height;
    uint32_t num_cols = leds->panel_width;

    mgos_universal_led_clear(leds);
    // set every column
    for (int y = 0; y < num_rows; y++) {
        for (int x = 0; x < num_cols; x++) {
            int x_pos = x + x_offset;
            int y_pos = y + y_offset;
            x_pos = wrap ? (x_pos % leds->pic_width) : x_pos;
            y_pos = wrap ? (y_pos % leds->pic_height) : y_pos;

            mgos_rgbleds_coords src;
            mgos_universal_led_rotate_coords(x_pos, y_pos, leds->pic_width, leds->pic_height, leds->rotate_col, &src);
            uint32_t pic_num_pixels = leds->pic_width * leds->pic_height;
            bool is_updown = mgos_sys_config_get_universal_led_updown();
            int new_x = is_updown ? leds->pic_width - 1 - src.x : src.x;
            int src_runner = ((src.y * src.w) + new_x) % pic_num_pixels;
            uint8_t* p_pix = leds->color_values + (src_runner * leds->pic_channels);
            tools_rgb_data out_pix;
            out_pix.r = p_pix[0];
            out_pix.g = p_pix[1];
            out_pix.b = p_pix[2];

            mgos_universal_led_plot_pixel(leds, x, y, out_pix, false);
        }
    }
}

void mgos_universal_led_unset(mgos_rgbleds* leds, int pix)
{
    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_WS2812:
    case MGOS_RGBLEDS_TYPE_APA102:
        mgos_neopixel_set((struct mgos_neopixel*)leds->driver, pix, 0x00, 0x00, 0x00);
        break;
    default:
        break;
    }
}

tools_rgb_data mgos_universal_led_get(mgos_rgbleds* leds, int pix, char* out, int len)
{
    tools_rgb_data out_pix;
    uint8_t* p = leds->data + (pix * leds->num_channels);

    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_WS2812:
    case MGOS_RGBLEDS_TYPE_APA102:
        switch (leds->color_order) {
        case MGOS_NEOPIXEL_ORDER_RGB:
            out_pix.r = p[0];
            out_pix.g = p[1];
            out_pix.b = p[2];
            break;
        case MGOS_NEOPIXEL_ORDER_GRB:
            out_pix.g = p[0];
            out_pix.r = p[1];
            out_pix.b = p[2];
            break;
        case MGOS_NEOPIXEL_ORDER_BGR:
            out_pix.b = p[0];
            out_pix.g = p[1];
            out_pix.r = p[2];
            break;
        default:
            LOG(LL_ERROR, ("Wrong order: %d", leds->color_order));
            break;
        }
        break;
    default:
        LOG(LL_ERROR, ("Unknown LED type: %d", leds->led_type));
        break;
    }

    if (len > 11) {
        sprintf(out, "%.03d,%.03d,%.03d", out_pix.r, out_pix.g, out_pix.b);
    }

    return out_pix;
}

tools_rgb_data mgos_universal_led_get_from_pos(mgos_rgbleds* leds, int x, int y, char* out, int len)
{
    uint16_t led_pos = mgos_universal_led_calc_pix_num(leds, x, y, false);
    return mgos_universal_led_get(leds, led_pos, out, len);
}

void mgos_universal_led_clear(mgos_rgbleds* leds)
{
    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_WS2812:
    case MGOS_RGBLEDS_TYPE_APA102:
        mgos_neopixel_clear((struct mgos_neopixel*)leds->driver);
        break;
    default:
        break;
    }
}

void mgos_universal_led_show(mgos_rgbleds* leds)
{
    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
        LOG(LL_DEBUG, ("Control LEDs over GPIO with bitbanging (WS2812)"));
        mgos_neopixel_show((struct mgos_neopixel*)leds->driver);
        break;
    case MGOS_RGBLEDS_TYPE_APA102:
        LOG(LL_DEBUG, ("Control LEDs over SPI (APA102)"));
        mgos_universal_led_transfer_spi(leds);
        break;
    case MGOS_RGBLEDS_TYPE_WS2812:
        LOG(LL_DEBUG, ("Control LEDs over ESP32 RMT"));
        ws2812_write_leds(((struct mgos_neopixel*)leds->driver)->data, leds->panel_height * leds->panel_height);
        break;
    default:
        break;
    }
}

void mgos_universal_led_transfer_spi(mgos_rgbleds* leds)
{
    uint16_t pix;
    int calc_brightness = (int)round(31.0 * leds->dim_all);
    calc_brightness = (calc_brightness <= 0) ? 1 : (calc_brightness > 31) ? 31 : calc_brightness;
    uint8_t brightness = leds->soft_dim ? 0x1F : (uint8_t)calc_brightness;
    uint32_t num_pixels = leds->panel_width * leds->panel_height;

    /* Global SPI instance is configured by the `spi` config section. */
    struct mgos_spi* spi = mgos_spi_get_global();
    if (spi == NULL) {
        LOG(LL_ERROR, ("SPI is not configured, make sure spi.enable is true"));
        return;
    }

    // Start Frame is included in the buffer
    // included: Reset frame - Only needed for SK9822, has no effect on APA102
    // included: End frame: 8+8*(leds >> 4) clock cycles
    uint8_t offset = leds->led_type == MGOS_RGBLEDS_TYPE_APA102 ? 1 : 0;
    uint32_t tx_len = 4 + (num_pixels * (leds->num_channels + offset)) + (8 + (num_pixels >> 4));
    uint8_t* tx_data = calloc(1, tx_len);
    for (pix = 0; pix < num_pixels; pix++) {
        uint8_t* p_color = leds->data + (pix * leds->num_channels);
        uint8_t* p_tx = (tx_data + 4) + (pix * (leds->num_channels + offset));
        if (leds->led_type == MGOS_RGBLEDS_TYPE_APA102) {
            p_tx[0] = brightness | 0xE0; // setting the brightness
            p_tx += offset;
        }
        p_tx[0] = p_color[0];
        p_tx[1] = p_color[1];
        p_tx[2] = p_color[2];
        if (leds->led_type == MGOS_RGBLEDS_TYPE_APA102) {
            p_tx -= offset;
        }
    }

    struct mgos_spi_txn txn = {
        .cs = 0, /* Use CS0 line as configured by cs0_gpio */
        .mode = 0,
        .freq = mgos_sys_config_get_universal_led_spi_freq(),
    };

    /* Half-duplex, command/response transaction setup */
    /* Transmit 1 byte from tx_data. */
    txn.hd.tx_len = tx_len;
    txn.hd.tx_data = tx_data;
    /* No dummy bytes necessary. */
    txn.hd.dummy_len = 0;
    /* Receive 3 bytes into rx_data. */
    txn.hd.rx_data = NULL;
    txn.hd.rx_len = 0;

    LOG(LL_DEBUG, ("Start SPI transaction."));
    if (!mgos_spi_run_txn(spi, false, &txn)) {
        LOG(LL_ERROR, ("SPI transaction failed"));
    }
    LOG(LL_DEBUG, ("Success: SPI transaction finished."));

    free(tx_data);
    return;
}

void mgos_universal_led_free(mgos_rgbleds* leds)
{
    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_WS2812:
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_APA102:
        mgos_neopixel_free((struct mgos_neopixel*)leds->driver);
        break;
    default:
        break;
    }

    if (leds->gaps_len) {
        free(leds->gaps);
        leds->gaps = NULL;
        leds->gaps_len = 0;
    }

    if (leds->lutR != NULL) {
        free(leds->lutR);
    }
    if (leds->lutG != NULL) {
        free(leds->lutG);
    }
    if (leds->lutB != NULL) {
        free(leds->lutB);
    }
    leds->lutR = NULL;
    leds->lutG = NULL;
    leds->lutB = NULL;

    free(leds->color_values);
    free(leds);
}

static void mgos_timer_cb(void* params)
{
    mgos_rgbleds* leds = (mgos_rgbleds*)params;
    LOG(LL_DEBUG, ("Call timer callback in C now with delay <%d> ... Callback: 0x%x", leds->timeout, (unsigned int)leds->callback));
    //mgos_universal_led_stop(leds);
    leds->callback(leds);
    //leds->timer_id = mgos_set_timer(leds->timeout, 0, mgos_timer_cb, leds);
}

void mgos_universal_led_one_shot(mgos_rgbleds* leds)
{
    if (leds->callback != NULL) {
        LOG(LL_DEBUG, ("One shot callback: 0x%x", (unsigned int)leds->callback));
        mgos_universal_led_stop(leds);
        leds->callback(leds);
    } else {
        LOG(LL_ERROR, ("Callback is not assigned! Quitting ..."));
    }
}

void mgos_universal_led_start_delayed(mgos_rgbleds* leds)
{
    if (leds->timer_id == 0) {
        if (leds->callback != NULL) {
            LOG(LL_INFO, ("Started timer in C now with delay <%d> ... Callback: 0x%x", leds->timeout, (unsigned int)leds->callback));
            leds->timer_id = mgos_set_timer(leds->timeout, 0, mgos_timer_cb, leds);
        } else {
            LOG(LL_ERROR, ("Callback is not assigned! Quitting ..."));
        }
    }
}

void mgos_universal_led_start(mgos_rgbleds* leds)
{
    if (leds->timer_id == 0) {
        if (leds->callback != NULL) {
            LOG(LL_INFO, ("Started timer in C now with runtime <%d> ... Callback: 0x%x", leds->timeout, (unsigned int)leds->callback));
            leds->timer_id = mgos_set_timer(leds->timeout, MGOS_TIMER_REPEAT | MGOS_TIMER_RUN_NOW, mgos_timer_cb, leds);
        } else {
            LOG(LL_ERROR, ("Callback is not assigned! Quitting ..."));
        }
    }
}

void mgos_universal_led_stop(mgos_rgbleds* leds)
{
    if (leds->timer_id != 0) {
        mgos_clear_timer(leds->timer_id);
        leds->timer_id = 0;
    }
}

bool mgos_RGB_LEDs_init(void)
{
    return true;
}
