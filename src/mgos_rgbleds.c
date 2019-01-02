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

mgos_rgbleds *mgos_rgbleds_create(int pin, int num_pixels, mgos_rgbleds_order order, rgbleds_callback callback)
{
  mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
  /* Keep in reset */
  mgos_gpio_write(pin, 0);

  mgos_rgbleds *np = calloc(1, sizeof(*np));
  np->callback = callback;
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
  np->col_pos = 0;
  np->pix_pos = 0;

  np->T0H = mgos_sys_config_get_rgbleds_timing_T0H();
  np->T1H = mgos_sys_config_get_rgbleds_timing_T1H();
  np->T0L = mgos_sys_config_get_rgbleds_timing_T0L();
  np->T1L = mgos_sys_config_get_rgbleds_timing_T1L();
  np->RES = mgos_sys_config_get_rgbleds_timing_RES();

  mgos_rgbleds_clear(np);
  return np;
}

void mgos_rgbleds_set(mgos_rgbleds *np, int i, int r, int g, int b) {
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

void mgos_rgbleds_get(mgos_rgbleds *np, int i, char *out, int len) {
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

void mgos_rgbleds_clear(mgos_rgbleds *np)
{
  memset(np->data, 0, np->num_pixels * NUM_CHANNELS);
}

void mgos_rgbleds_show(mgos_rgbleds *np) {
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(np->RES);
  mgos_bitbang_write_bits(np->pin, MGOS_DELAY_100NSEC, np->T0H, np->T0L, np->T1H, np->T1L, np->data, np->num_pixels * NUM_CHANNELS);
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(np->RES);
  mgos_gpio_write(np->pin, 1);
}

void mgos_rgbleds_show_fast(mgos_rgbleds *np)
{
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(np->RES);
  mgos_bitbang_write_bits(np->pin, MGOS_DELAY_100NSEC, np->T0H, np->T0L, np->T1H, np->T1L, np->data, np->counter * NUM_CHANNELS);
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(np->RES);
  mgos_gpio_write(np->pin, 1);
}

void mgos_rgbleds_free(mgos_rgbleds *np)
{
  free(np->data);
  free(np->color_shadows);
  free(np->color_values);
  free(np);
}

void mgos_rgbleds_prep_colors(mgos_rgbleds *np, int pix, uint8_t r, uint8_t g, uint8_t b, int col_num)
{
  if (np->color_values != NULL) {
    uint8_t *p_color = np->color_values + (pix * NUM_CHANNELS);
    p_color[0] = r;
    p_color[1] = g;
    p_color[2] = b;
  }
  np->col_count = col_num;
}

void mgos_rgbleds_start(mgos_rgbleds *np, int timeout) {
  if (np->timer_id == 0) {
    np->col_pos = 0;
    if (np->callback != NULL) {
      np->timer_id = mgos_set_timer(timeout, MGOS_TIMER_REPEAT, np->callback, np);
    }
  }
}

void mgos_rgbleds_stop(mgos_rgbleds *np)
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
