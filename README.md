# Adafruit_NeoPixel_ESP8266_RTOS_SDK

Moderately modified version of the [Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) Arduino library to allow it to properly interface with ESP8266_RTOS_SDK as a component. 

## Major changes:

- Addition of component.mk with specific preprocessor flags.
- Change of Arduino framework functions (pinMode(), millis(), etc.) to native ESP8266 and FreeRTOS functions (gpio_config(), esp_timer_get_time(), etc.).
- Change of Adafruit_NeoPixel C++ class methods (begin(), setPixelColor(), etc.) to account for the class-less nature of C functions (anp_begin(), anp_setPixelColor_RGB(), etc.); this includes the constructor as well.
- Fix includes to also match the native headers.
- C++ guards at both ends of the relevant files.

## Usage:

- For clarity, `anp` is short for `Adafruit_NeoPixel`.
- The class `Adafruit_NeoPixel` has been replaced with the `AnpStrip` struct. The constructor has also been replaced.
  - Example: `Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800)` is now `AnpStrip *pixels`. The function `new_AnpStrip()` must be subsequently called: `pixels = new_AnpStrip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800)`.
  - Note that the other two constructors have been removed for now.
- The variables in `Adafruit_NeoPixel` transition one-to-one onto `AnpStrip`, but all are public now. However, variables are accessed with `->` instead of `.` (since pixels is a pointer).
  -Example: `pixels.numLEDs` is now `pixels->numLEDs`.
- All methods of the class `Adafruit_NeoPixel` are now functions which take in a pointer to an `AnpStrip` struct as the first parameter; the rest of the parameters are unchanged. In addition, all functions are prefixed with `anp_` for better readability.
  - Example: `pixels.begin()` is now `anp_begin(strip)`.
- Some function names have been changed due to the lack of overloading in C. Most of these are related to color operations.
  - Example: `pixels.setPixelColor(i, pixels.Color(0, 150, 0))` is now `anp_setPixelColor_C(pixels, i, anp_Color_RGB(0, 150, 0))`.
  - Here, the suffix `_C` on `anp_setPixelColor_C()` means that the function takes in a `Color`; more specifically, the `Color` here is from `anp_Color_RGB()` (which would've been a `pixels.Color()` previously). The suffixes `_RGB` and `_RGBW` also exist for `anp_setPixelColor`. `anp_ColorHSV()` is largely unchanged from before.

Documentation of most functions may be found in the source code of Adafruit_NeoPixel. Most documentation has been removed in this version for the sake of brevity.

Please refer to Adafruit_NeoPixel for additional information regarding the license and related commentary.
