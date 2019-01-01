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

struct mgos_rgbleds *mgos_rgbleds_create(int pin, int num_pixels,
                                           enum mgos_rgbleds_order order) {
  mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
  /* Keep in reset */
  mgos_gpio_write(pin, 0);

  struct mgos_rgbleds *np = calloc(1, sizeof(*np));
  np->pin = pin;
  np->num_pixels = num_pixels;
  np->order = order;
  np->data = malloc(num_pixels * NUM_CHANNELS);
  np->col_count = num_pixels;
  np->color_values = malloc(np->num_pixels * NUM_CHANNELS);
  memset(np->color_values, 0, np->num_pixels * NUM_CHANNELS);
  np->color_shadows = malloc(np->num_pixels * NUM_CHANNELS);
  memset(np->color_shadows, 0, np->num_pixels * NUM_CHANNELS);
  np->counter = 0;
  np->timer_id = 0;
  np->single_mode = false;
  np->shift = num_pixels;
  mgos_rgbleds_clear(np);
  return np;
}

void mgos_rgbleds_set(struct mgos_rgbleds *np, int i, int r, int g, int b) {
  uint8_t *p = np->data + i * NUM_CHANNELS;
  switch (np->order) {
    case MGOS_RGBLEDS_ORDER_RGB:
      p[0] = r;
      p[1] = g;
      p[2] = b;
      break;

    case MGOS_RGBLEDS_ORDER_GRB:
      p[0] = g;
      p[1] = r;
      p[2] = b;
      break;

    case MGOS_RGBLEDS_ORDER_BGR:
      p[0] = b;
      p[1] = g;
      p[2] = r;
      break;

    default:
      LOG(LL_ERROR, ("Wrong order: %d", np->order));
      break;
  }
}

void mgos_rgbleds_get(struct mgos_rgbleds *np, int i, char *out, int len) {
  uint8_t *p = np->data + i * NUM_CHANNELS;
  int r,g,b;
  switch (np->order) {
    case MGOS_RGBLEDS_ORDER_RGB:
      r = p[0];
      g = p[1];
      b = p[2];
      break;

    case MGOS_RGBLEDS_ORDER_GRB:
      g = p[0];
      r = p[1];
      b = p[2];
      break;

    case MGOS_RGBLEDS_ORDER_BGR:
      b = p[0];
      g = p[1];
      r = p[2];
      break;

    default:
      LOG(LL_ERROR, ("Wrong order: %d", np->order));
      // don't print a result ...
      len = 0;
      break;
  }

  if (len >= 12) {
    sprintf(out, "%3d,%3d,%3d", r, g, b);
  }
}

void mgos_rgbleds_clear(struct mgos_rgbleds *np)
{
  memset(np->data, 0, np->num_pixels * NUM_CHANNELS);
}

void mgos_rgbleds_show(struct mgos_rgbleds *np) {
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(60);
  mgos_bitbang_write_bits(np->pin, MGOS_DELAY_100NSEC, 3, 8, 8, 6, np->data,
                          np->num_pixels * NUM_CHANNELS);
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(60);
  mgos_gpio_write(np->pin, 1);
}

void mgos_rgbleds_show_fast(struct mgos_rgbleds *np)
{
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(60);
  mgos_bitbang_write_bits(np->pin, MGOS_DELAY_100NSEC, 3, 8, 8, 6, np->data, (np->counter + 1) * NUM_CHANNELS);
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(60);
  mgos_gpio_write(np->pin, 1);
}

void mgos_rgbleds_free(struct mgos_rgbleds *np)
{
  free(np->data);
  free(np->color_shadows);
  free(np->color_values);
  free(np);
}

void mgos_rgbleds_prep_colors(struct mgos_rgbleds *np, int pix, uint8_t r, uint8_t g, uint8_t b, int col_num)
{
  if (np->color_values != NULL) {
    uint8_t *p_color = np->color_values + (pix * NUM_CHANNELS);
    p_color[0] = r;
    p_color[1] = g;
    p_color[2] = b;
  }
  np->col_count = col_num;
}

void mgos_rgbleds_process(void *param)
{
  struct mgos_rgbleds *np = (struct mgos_rgbleds *) param;
  np->counter = np->counter >= np->col_count ? 0 : np->counter;
  if (np->counter == 0 && np->single_mode) {
    mgos_rgbleds_clear(np);
  }
  
  int pix = np->counter;
  int last = pix - 1;
  
  //uint8_t *p_pix = np->data + (pix * NUM_CHANNELS);
  uint8_t *p_pix_color = np->color_values + (pix * NUM_CHANNELS);
  // LOG(LL_INFO, ("mgos_rgbleds_process: pix - <%d>, lst - <%d>, counter - <%d>", pix, last, np->counter));

  if (np->single_mode) {
    uint8_t *p_pix_shadow = np->color_shadows + (pix * NUM_CHANNELS);
    if (last > -1) {
      uint8_t *p_last_shadow = np->color_shadows + (last * NUM_CHANNELS);
      // LOG(LL_INFO, ("mgos_rgbleds_process: set (last) - R - <%d>, G - <%d>, B - <%d>", p_last_shadow[0], p_last_shadow[1], p_last_shadow[2]));
      mgos_rgbleds_set(np, last, p_last_shadow[0], p_last_shadow[1], p_last_shadow[2]);
    }
    memcpy(p_pix_shadow, p_pix_color, NUM_CHANNELS);
  } else {
    int runner = 0, out = 0;
    for (runner = 0; runner < np->num_pixels; runner++) {
      out = (runner + pix) % np->num_pixels;
      p_pix_color = np->color_values + (runner * NUM_CHANNELS);
      mgos_rgbleds_set(np, out, p_pix_color[0], p_pix_color[1], p_pix_color[2]);
    }
  }

  // LOG(LL_INFO, ("mgos_rgbleds_process: set (pix) - R - <%d>, G - <%d>, B - <%d>", p_pix_color[0], p_pix_color[1], p_pix_color[2]));
//  mgos_rgbleds_set(np, pix, p_pix_color[0], p_pix_color[1], p_pix_color[2]);
  mgos_rgbleds_show(np);
  np->counter++;
  np->shift--;
}

void mgos_rgbleds_start(struct mgos_rgbleds *np, int timeout) {
  if (np->timer_id == 0) {
    np->shift = 0;
    np->timer_id = mgos_set_timer(timeout, MGOS_TIMER_REPEAT, mgos_rgbleds_process, np);
  }
}

void mgos_rgbleds_stop(struct mgos_rgbleds *np)
{
  if (np->timer_id != 0) {
    mgos_clear_timer(np->timer_id);
    np->timer_id = 0;
  }
}

bool mgos_RGB_LEDs_init(void)
{
  return true;
}
