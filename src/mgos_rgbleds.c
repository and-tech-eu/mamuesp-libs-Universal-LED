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

#include "mgos_rgbleds.h"
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
mgos_rgbleds* mgos_rgbleds_get_global()
{
    if (global_leds == NULL) {
        global_leds = mgos_rgbleds_create();
    }
    return global_leds;
}

mgos_rgbleds* mgos_rgbleds_create()
{
    mgos_rgbleds* leds = calloc(1, sizeof(mgos_rgbleds));
    int pin = mgos_sys_config_get_rgbleds_pin();

    leds->color_file = (char*)mgos_sys_config_get_rgbleds_color_file();
    leds->color_count = mgos_sys_config_get_rgbleds_color_count();
    leds->color_order = mgos_sys_config_get_rgbleds_color_order();
    leds->toggle_odd = mgos_sys_config_get_rgbleds_toggle_odd();
    leds->rotate_col = mgos_sys_config_get_rgbleds_rotate_col();
    leds->rotate_out = mgos_sys_config_get_rgbleds_rotate_out();
    leds->num_pixels = mgos_sys_config_get_rgbleds_num_pixels();
    leds->num_channels = mgos_sys_config_get_rgbleds_num_channels();
    leds->panel_width = mgos_sys_config_get_rgbleds_panel_width();
    leds->panel_height = mgos_sys_config_get_rgbleds_panel_height();
    leds->dim_all = mgos_sys_config_get_rgbleds_dim_all();
    leds->dim_red = mgos_sys_config_get_rgbleds_dim_red();
    leds->dim_green = mgos_sys_config_get_rgbleds_dim_green();
    leds->dim_blue = mgos_sys_config_get_rgbleds_dim_blue();
    leds->led_type = mgos_sys_config_get_rgbleds_led_type();
    leds->single_mode = mgos_sys_config_get_rgbleds_single_mode();
    leds->invert_colors = mgos_sys_config_get_rgbleds_invert_colors();
    leds->timeout = mgos_sys_config_get_rgbleds_timeout();
    leds->data_len = leds->num_pixels * leds->num_channels;

    leds->lutR = (uint8_t*)gammaR8;
    leds->lutG = (uint8_t*)gammaG8;
    leds->lutB = (uint8_t*)gammaB8;
    leds->color_shadows = calloc(leds->num_pixels, leds->num_channels);
    leds->color_values = calloc(1, leds->num_pixels * leds->num_channels);
    memset(leds->color_values, 0x0f, leds->num_pixels * leds->num_channels);

    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_WS2812:
    case MGOS_RGBLEDS_TYPE_APA102:
        leds->driver = mgos_neopixel_create(pin, leds->num_pixels, leds->color_order, leds->num_channels);
        leds->data = ((struct mgos_neopixel*)leds->driver)->data;
        break;
    default:
        break;
    }

    mgos_rgbleds_clear(leds);
    mgos_rgbleds_show(leds);
    return leds;
}

void mgos_rgbleds_get_colors(mgos_rgbleds* leds)
{
    if (leds->color_file != NULL) {
        bmpread_t* p_bmp_out = calloc(1, sizeof(bmpread_t));
        if (bmpread(leds->color_file, BMPREAD_TOP_DOWN | BMPREAD_BYTE_ALIGN, p_bmp_out) > 0) {
            leds->pic_width = p_bmp_out->width;
            leds->pic_height = p_bmp_out->height;
            leds->num_pixels = leds->panel_width * leds->panel_height;
            leds->data_len = leds->pic_width * leds->pic_height * 3;
            leds->color_shadows = realloc(leds->color_shadows, leds->data_len);
            leds->color_values = realloc(leds->color_values, leds->data_len);
            memcpy(leds->color_values, p_bmp_out->data, leds->data_len);
            mgos_rgbleds_adapt_colors(leds);
            memcpy(leds->color_shadows, leds->color_values, leds->data_len);
            bmpread_free(p_bmp_out);
            free(p_bmp_out);
            LOG(LL_INFO, ("Bitmap file <%s> loaded and processed!", leds->color_file));
        } else {
            LOG(LL_ERROR, ("Bitmap file <%s> could not be loaded!", leds->color_file));
        }
    } else {
        LOG(LL_ERROR, ("Bitmap file name is NULL, so colors could not be loaded!"));
    }
}

void mgos_rgbleds_rotate_colors(mgos_rgbleds* leds)
{
    uint8_t* dest = calloc(leds->data_len, 1);
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

void mgos_rgbleds_adapt_colors(mgos_rgbleds* leds)
{
    uint8_t col_pos = 0;
    uint32_t len = leds->data_len;
    uint8_t* p_col = leds->color_values;
    while (len--) {
        int val = *p_col;
        double dim = leds->dim_all;
        uint8_t* lut = NULL;
        val = leds->invert_colors ? (255 - val) : val;
        switch (col_pos) {
        case 0:
            dim *= leds->dim_red;
            lut = leds->lutR;
            break;
        case 1:
            dim *= leds->dim_green;
            lut = leds->lutG;
            break;
        case 2:
            dim *= leds->dim_blue;
            lut = leds->lutB;
            break;
        default:
            break;
        }
        if (lut != NULL) {
            val = (int)lut[val];
        }
        val = (int)round(val * dim);
        val = (val > 255) ? 255 : val;
        val = (val < 0) ? 0 : val;
        *p_col++ = val;
        if (++col_pos == 3) {
            col_pos = 0;
        }
    }
}

void mgos_rgbleds_adapt_viewport(mgos_rgbleds* leds, int vp_x, int vp_y, int col_x, int col_y, bool do_black)
{
    mgos_rgbleds_coords col;
    mgos_rgbleds_rotate_coords(col_x, col_y, leds->pic_width, leds->pic_height, leds->rotate_col, &col);
    int col_runner = ((col.y * col.w) + (col.x % col.w)) % leds->data_len;
    uint8_t black[4] = { 0, 0, 0, 0 };
    uint8_t* p_pix = do_black ? black : leds->color_values + (col_runner * leds->num_channels);

    mgos_rgbleds_coords dst;
    mgos_rgbleds_rotate_coords(vp_x, vp_y, leds->panel_width, leds->panel_height, leds->rotate_out, &dst);
    bool isOdd = ((dst.y & 0x01) == 1);
    int corr_x = (leds->toggle_odd && isOdd) ? (dst.w - (dst.x + 1)) : dst.x;
    int runner = (dst.y * dst.w) + corr_x;

    mgos_rgbleds_set(leds, runner, p_pix[0], p_pix[1], p_pix[2]);
}

void mgos_rgbleds_rotate_coords(int x, int y, int w, int h, int deg, mgos_rgbleds_coords* out)
{
    int dst_x, dst_y, dst_w, dst_h;
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
}

void mgos_rgbleds_set(mgos_rgbleds* leds, int pix, int r, int g, int b)
{
    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_WS2812:
    case MGOS_RGBLEDS_TYPE_APA102:
        mgos_neopixel_set((struct mgos_neopixel*)leds->driver, pix, r, g, b);
        break;
    default:
        break;
    }
}

void mgos_rgbleds_get(mgos_rgbleds* leds, int pix, char* out, int len)
{
    uint8_t* p;
    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_WS2812:
    case MGOS_RGBLEDS_TYPE_APA102:
        p = leds->data + pix * leds->num_channels;
        if (len > 11) {
            sprintf(out, "%.03d,%.03d,%.03d", p[0], p[1], p[2]);
        }
        break;
    default:
        break;
    }
}

void mgos_rgbleds_clear(mgos_rgbleds* leds)
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

void mgos_rgbleds_show(mgos_rgbleds* leds)
{
    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_WS2812:
        LOG(LL_DEBUG, ("Control LEDs over GPIO with bitbanging (WS2812)"));
        mgos_neopixel_show((struct mgos_neopixel*)leds->driver);
        break;
    case MGOS_RGBLEDS_TYPE_APA102:
        LOG(LL_DEBUG, ("Control LEDs over SPI (APA102"));
        mgos_rgbleds_transfer_spi(leds);
        break;
    default:
        break;
    }
}

void mgos_rgbleds_transfer_spi(mgos_rgbleds* leds)
{
    uint16_t pix;
    uint8_t brightness = 0xFF; //(leds->led_type == MGOS_RGBLEDS_TYPE_APA102) ? ((uint8_t)round((0x1F  *leds->dim) / 100.0) | 0xE0) : 0x1F;

    /* Global SPI instance is configured by the `spi` config section. */
    struct mgos_spi* spi = mgos_spi_get_global();
    if (spi == NULL) {
        LOG(LL_ERROR, ("SPI is not configured, make sure spi.enable is true"));
        return;
    }

    // Start Frame is included in the buffer
    // included: Reset frame - Only needed for SK9822, has no effect on APA102
    // included: End frame: 8+8*(leds >> 4) clock cycles
    uint32_t tx_len = (leds->num_pixels * (leds->num_channels + 1)) + (8 + (8 * (leds->num_pixels >> 4))) + 8;
    uint8_t* tx_data = calloc(1, tx_len);

    for (pix = 0; pix < leds->num_pixels; pix++) {
        uint8_t* p_color = leds->data + (pix * leds->num_channels);
        uint8_t* p_tx = (tx_data + 4) + (pix * leds->num_channels);
        if (leds->led_type == MGOS_RGBLEDS_TYPE_APA102) {
            p_tx[0] = brightness; // Maximum global brightness
            p_tx++;
        }
        memcpy(p_tx, p_color, 3);
        p_tx--;
        //int ansi_color = rgb_to_ansi(p_tx[1], p_tx[2], p_tx[3]);
        //char* fmt = "[SPI data]%c[%dm Ansi: %.03d, Len: %d - Brightness: 0x%.02x,  Colors: 0x%.02x, 0x%.02x, 0x%.02x";
        char* fmt = "[SPI data] Len: %d - Brightness: 0x%.02x, Colors: 0x%.02x, 0x%.02x, 0x%.02x";
        //LOG(LL_INFO, (fmt, (unsigned int)0x1B, (unsigned int)ansi_color, (unsigned int)ansi_color, tx_len, p_tx[0], p_tx[1], p_tx[2], p_tx[3]));
        LOG(LL_DEBUG, (fmt, tx_len, p_tx[0], p_tx[1], p_tx[2], p_tx[3]));
    }

    struct mgos_spi_txn txn = {
        .cs = 0, /* Use CS0 line as configured by cs0_gpio */
        .mode = 0,
        .freq = 10000000,
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

void mgos_rgbleds_free(mgos_rgbleds* leds)
{
    switch (leds->led_type) {
    case MGOS_RGBLEDS_TYPE_NEOPIXEL:
    case MGOS_RGBLEDS_TYPE_WS2812:
    case MGOS_RGBLEDS_TYPE_APA102:
        mgos_neopixel_free((struct mgos_neopixel*)leds->driver);
        break;
    default:
        break;
    }

    //free(leds->color_shadows);
    free(leds->color_values);
    free(leds);
}

static void mgos_timer_cb(void* params)
{
    mgos_rgbleds* leds = (mgos_rgbleds*)params;
    LOG(LL_DEBUG, ("Call timer callback in C now with delay <%d> ... Callback: 0x%x", leds->timeout, (unsigned int)leds->callback));
    mgos_rgbleds_stop(leds);
    leds->callback(leds);
    leds->timer_id = mgos_set_timer(leds->timeout, 0, mgos_timer_cb, leds);
}

void mgos_rgbleds_start(mgos_rgbleds* leds)
{
    mgos_rgbleds_get_colors(leds);
    if (leds->timer_id == 0) {
        if (leds->callback != NULL) {
            LOG(LL_INFO, ("Started timer in C now with delay <%d> ... Callback: 0x%x", leds->timeout, (unsigned int)leds->callback));
            leds->timer_id = mgos_set_timer(leds->timeout, 0, mgos_timer_cb, leds);
        } else {
            LOG(LL_ERROR, ("Callback is not assigned! Quitting ..."));
        }
    }
}

void mgos_rgbleds_stop(mgos_rgbleds* leds)
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
