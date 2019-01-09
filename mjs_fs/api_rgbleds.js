load("api_bitbang.js");
load("api_gpio.js");
load("api_sys.js");

let RGB_LEDs = {
  RGB: 0,
  GRB: 1,
  BGR: 2,

  NEOPIXEL: 0,
  WS2812: 1,
  APA102: 2,
  RGB_PWM: 3,
  RGBW_PWM: 4,

  // ## **`RGB_LEDs.create(pin, numPixels, order)`**
  // Create and return a NeoPixel strip object. Example:
  // ```javascript
  // let pin = 5, numPixels = 16, colorOrder = RGB_LEDs.GRB;
  // let strip = RGB_LEDs.create(pin, numPixels, colorOrder);
  // strip.setPixel(0 /* pixel */, 12, 34, 56);
  // strip.show();
  //
  // strip.clear();
  // strip.setPixel(1 /* pixel */, 12, 34, 56);
  // strip.show();
  // ```
  create: function () {
    return Object.create({

      _i: RGB_LEDs._getGlobal(),

      setPixel: RGB_LEDs.setPixel,
      getPixel: RGB_LEDs.getPixel,
      clear: RGB_LEDs.clear,
      show: RGB_LEDs.show,
      prepCols: RGB_LEDs.prepCols,
      start: RGB_LEDs.start,
      stop: RGB_LEDs.stop,
      getInstance: RGB_LEDs.getInstance
    });
  },

  // ## **`strip.setPixel(i, r, g, b)`**
  // Set i-th's pixel's RGB value.
  // Note that this only affects in-memory value of the pixel.
  setPixel: function (pix, r, g, b) {
    if (this._i !== null && this._i !== undefined) {
      RGB_LEDs._set(this._i, pix, r, g, b);
    } else {
      Log.error("setPixel: Instance of RGB_LEDs is NULL!");
    }
  },

  // ## **`strip.getPixel(i)`**
  // Returns i-th's pixel's RGB value as CSV string.
  // Note that this only reads in-memory value of the pixel.
  getPixel: function (i) {
    if (this._i !== null && this._i !== undefined) {
      let out = '            ';
      RGB_LEDs._get(this._i, i, out, 12);
      let res = out.slice(0, 11);
      return res;
    } else {
      Log.error("getPixel: Instance of RGB_LEDs is NULL!");
    }
    return '';
  },

  // ## **`strip.clear()`**
  // Clear in-memory values of the pixels.
  clear: function () {
    if (this._i !== null && this._i !== undefined) {
      RGB_LEDs._clear(this._i);
    } else {
      Log.error("clear: Instance of RGB_LEDs is NULL!");
    }
  },

  // ## **`strip.show()`**
  // Output values of the pixels.
  show: function () {
    if (this._i !== null && this._i !== undefined) {
      RGB_LEDs._show(this._i);
    } else {
      Log.error("show: Instance of RGB_LEDs is NULL!");
    }
  },

  start: function () {
    if (this._i !== null && this._i !== undefined) {
      RGB_LEDs._start(this._i);
    } else {
      Log.error("start: Instance of RGB_LEDs is NULL!");
    }
  },

  stop: function () {
    if (this._i !== null && this._i !== undefined) {
      RGB_LEDs._stop(this._i);
    } else {
      Log.error("stop: Instance of RGB_LEDs is NULL!");
    }
  },

  getInstance: function () {
    if (this._i !== null && this._i !== undefined) {
      return this._i;
    } else {
      Log.error("getInstance: Instance of RGB_LEDs is NULL!");
      return {};
    }
  },

  _c: ffi('void *mgos_rgbleds_create()'),
  _global: ffi('void *mgos_rgbleds_get_global()'),
  _set: ffi('void mgos_rgbleds_set(void *, int, int, int, int)'),
  _get: ffi('void mgos_rgbleds_get(void *, int, char *, int)'),
  _clear: ffi('void mgos_rgbleds_clear(void *)'),
  _show: ffi('void mgos_rgbleds_show(void *)'),
  _start: ffi('void mgos_rgbleds_start(void *)'),
  _stop: ffi('void mgos_rgbleds_stop(void *)')
};
