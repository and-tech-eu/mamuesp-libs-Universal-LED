load("api_bitbang.js");
load("api_gpio.js");
load("api_sys.js");

let RGB_LEDs = {
  RGB: 0,
  GRB: 1,
  BGR: 2,

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
  create: function(pin, numPixels, order) {
    return Object.create({
      _i: RGB_LEDs._c(pin, numPixels, order, null),

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
  setPixel: function(i, r, g, b) {
    RGB_LEDs._set(this._i, i, r, g, b);
  },

  // ## **`strip.getPixel(i)`**
  // Returns i-th's pixel's RGB value as CSV string.
  // Note that this only reads in-memory value of the pixel.
  getPixel: function (i) {
    let out = '            ';
    RGB_LEDs._get(this._i, i, out, 12);
    let res =  out.slice(0, 11);
    return res;
  },

  // ## **`strip.clear()`**
  // Clear in-memory values of the pixels.
  clear: function() {
    RGB_LEDs._clear(this._i);
  },

  // ## **`strip.show()`**
  // Output values of the pixels.
  show: function() {
    RGB_LEDs._show(this._i);
  },

  prepCols: function(pix, r, g, b, num) {
    RGB_LEDs._prepCols(this._i, pix, r, g, b, num);
  },
  
  start: function (timeout) {
    RGB_LEDs._start(this._i, timeout);
   },
  
  stop: function () {
    RGB_LEDs._stop(this._i);
   },

  getInstance: function () {
    return this._i;
  },

  _c: ffi('void *mgos_rgbleds_create(int, int, int, void *)'),
  _set: ffi('void mgos_rgbleds_set(void *, int, int, int, int)'),
  _get: ffi('void mgos_rgbleds_get(void *, int, char *, int)'),
  _clear: ffi('void mgos_rgbleds_clear(void *)'),
  _show: ffi('void mgos_rgbleds_show(void *)'),
  _prepCols: ffi('void mgos_rgbleds_prep_colors(void *, int, int, int, int, int)'),
  _start: ffi('void mgos_rgbleds_start(void *, int)'),
  _stop: ffi('void mgos_rgbleds_stop(void *)')
};
