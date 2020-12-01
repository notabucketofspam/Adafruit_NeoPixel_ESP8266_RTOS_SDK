

#include <anp_component.h>

#ifdef __cplusplus
extern "C" {
#endif

// Functions added to interface with ESP8266_RTOS_SDK
// anp = Adafrupt_NeoPixel
void anp_pinMode(uint32_t pin, uint32_t pin_mode) {
  gpio_mode_t gpio_mode = pin_mode ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
  gpio_config_t config = {
    .pin_bit_mask = BIT(pin),
    .mode = gpio_mode,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&config);
}
void anp_digitalWrite(uint32_t pin, uint32_t pin_level) {
  gpio_set_level((gpio_num_t) pin, pin_level);
}
// https://stackoverflow.com/questions/3774193/constructor-for-structs-in-c
AnpStrip *new_AnpStrip(uint16_t n, uint16_t p, neoPixelType t) {
  AnpStrip *strip = malloc(sizeof(AnpStrip));
  strip->begun = false;
  strip->brightness = 0;
  strip->pixels = NULL;
  strip->endTime = 0;
  anp_updateType(strip, t);
  anp_updateLength(strip, n);
  anp_setPin(strip, p);
  return strip;
}
void anp_begin(AnpStrip *strip) {
  if(strip->pin >= 0) {
    anp_pinMode(strip->pin, 1);
    anp_digitalWrite(strip->pin, 0);
  }
  strip->begun = true;
}
// ESP8266 show() is external to enforce IRAM_ATTR execution
extern void IRAM_ATTR anp_espShow(uint16_t pin, uint8_t *pixels, uint32_t numBytes, uint8_t type);
void anp_show(AnpStrip *strip) {
  if(!strip->pixels)
    return;
  while(!anp_canShow(strip));
  anp_espShow(strip->pin, strip->pixels, strip->numBytes, strip->is800KHz);
  strip->endTime = esp_timer_get_time();
}
bool anp_canShow(AnpStrip *strip) {
  if (strip->endTime > esp_timer_get_time()) {
    strip->endTime = esp_timer_get_time();
  }
  return (esp_timer_get_time() - strip->endTime) >= 300L;
}
void anp_setPin(AnpStrip *strip, uint16_t p) {
  if(strip->begun && (strip->pin >= 0))
    anp_pinMode(strip->pin, 0);
  strip->pin = p;
  if(strip->begun) {
    anp_pinMode(p, 1);
    anp_digitalWrite(p, 0);
  }
}
void anp_setPixelColor_RGB(AnpStrip *strip, uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if(n < strip->numLEDs) {
    if(strip->brightness) { // See notes in setBrightness()
      r = (r * strip->brightness) >> 8;
      g = (g * strip->brightness) >> 8;
      b = (b * strip->brightness) >> 8;
    }
    uint8_t *p;
    if(strip->wOffset == strip->rOffset) { // Is an RGB-type strip
      p = &strip->pixels[n * 3];    // 3 bytes per pixel
    } else {                 // Is a WRGB-type strip
      p = &strip->pixels[n * 4];    // 4 bytes per pixel
      p[strip->wOffset] = 0;        // But only R,G,B passed -- set W to 0
    }
    p[strip->rOffset] = r;          // R,G,B always stored
    p[strip->gOffset] = g;
    p[strip->bOffset] = b;
  }
}
void anp_setPixelColor_RGBW(AnpStrip *strip, uint16_t n, uint8_t r, uint8_t g, uint8_t b,  uint8_t w) {
  if(n < strip->numLEDs) {
    if(strip->brightness) { // See notes in setBrightness()
      r = (r * strip->brightness) >> 8;
      g = (g * strip->brightness) >> 8;
      b = (b * strip->brightness) >> 8;
      w = (w * strip->brightness) >> 8;
    }
    uint8_t *p;
    if(strip->wOffset == strip->rOffset) { // Is an RGB-type strip
      p = &strip->pixels[n * 3];    // 3 bytes per pixel (ignore W)
    } else {                 // Is a WRGB-type strip
      p = &strip->pixels[n * 4];    // 4 bytes per pixel
      p[strip->wOffset] = w;        // Store W
    }
    p[strip->rOffset] = r;          // Store R,G,B
    p[strip->gOffset] = g;
    p[strip->bOffset] = b;
  }
}
void anp_setPixelColor_C(AnpStrip *strip, uint16_t n, uint32_t c) {
  if(n < strip->numLEDs) {
    uint8_t *p,
      r = (uint8_t)(c >> 16),
      g = (uint8_t)(c >>  8),
      b = (uint8_t)c;
    if(strip->brightness) { // See notes in setBrightness()
      r = (r * strip->brightness) >> 8;
      g = (g * strip->brightness) >> 8;
      b = (b * strip->brightness) >> 8;
    }
    if(strip->wOffset == strip->rOffset) {
      p = &strip->pixels[n * 3];
    } else {
      p = &strip->pixels[n * 4];
      uint8_t w = (uint8_t)(c >> 24);
      p[strip->wOffset] = strip->brightness ? ((w * strip->brightness) >> 8) : w;
    }
    p[strip->rOffset] = r;
    p[strip->gOffset] = g;
    p[strip->bOffset] = b;
  }
}
void anp_fill(AnpStrip *strip, uint32_t c, uint16_t first, uint16_t count) {
  uint16_t i, end;
  if(first >= strip->numLEDs) {
    return; // If first LED is past end of strip, nothing to do
  }
  // Calculate the index ONE AFTER the last pixel to fill
  if(count == 0) {
    // Fill to end of strip
    end = strip->numLEDs;
  } else {
    // Ensure that the loop won't go past the last pixel
    end = first + count;
    if(end > strip->numLEDs) end = strip->numLEDs;
  }
  for(i = first; i < end; i++) {
    anp_setPixelColor_C(strip, i, c);
  }
}
void anp_setBrightness(AnpStrip *strip, uint8_t b) {
  uint8_t newBrightness = b + 1;
  if(newBrightness != strip->brightness) {
    uint8_t  c,
      *ptr           = strip->pixels,
       oldBrightness = strip->brightness - 1; // De-wrap old brightness value
    uint16_t scale;
    if(oldBrightness == 0) scale = 0; // Avoid /0
    else if(b == 255) scale = 65535 / oldBrightness;
    else scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
    for(uint16_t i=0; i<strip->numBytes; i++) {
      c      = *ptr;
      *ptr++ = (c * scale) >> 8;
    }
    strip->brightness = newBrightness;
  }
}
void anp_clear(AnpStrip *strip) {
  memset(strip->pixels, 0, strip->numBytes);
}
void anp_updateLength(AnpStrip *strip, uint16_t n) {
  free(strip->pixels); // Free existing data (if any)
  // Allocate new data -- note: ALL PIXELS ARE CLEARED
  strip->numBytes = n * ((strip->wOffset == strip->rOffset) ? 3 : 4);
  if((strip->pixels = (uint8_t *)malloc(strip->numBytes))) {
    memset(strip->pixels, 0, strip->numBytes);
    strip->numLEDs = n;
  } else {
    strip->numLEDs = strip->numBytes = 0;
  }
}
void anp_updateType(AnpStrip *strip, neoPixelType t) {
  bool oldThreeBytesPerPixel = (strip->wOffset == strip->rOffset); // false if RGBW
  strip->wOffset = (t >> 6) & 0b11; // See notes in header file
  strip->rOffset = (t >> 4) & 0b11; // regarding R/G/B/W offsets
  strip->gOffset = (t >> 2) & 0b11;
  strip->bOffset =  t       & 0b11;
#if defined(NEO_KHZ400)
  strip->is800KHz = (t < 256);      // 400 KHz flag is 1<<8
#endif
  // If bytes-per-pixel has changed (and pixel data was previously
  // allocated), re-allocate to new size. Will clear any data.
  if(strip->pixels) {
    bool newThreeBytesPerPixel = (strip->wOffset == strip->rOffset);
    if(newThreeBytesPerPixel != oldThreeBytesPerPixel)
      anp_updateLength(strip, strip->numLEDs);
  }
}
uint8_t *anp_getPixels(AnpStrip *strip) {
  return strip->pixels;
}
uint8_t anp_getBrightness(AnpStrip *strip) {
  return strip->brightness - 1;
}
int16_t anp_getPin(AnpStrip *strip) {
  return strip->pin;
}
uint16_t anp_numPixels(AnpStrip *strip) {
  return strip->numLEDs;
}
uint32_t anp_getPixelColor(AnpStrip *strip, uint16_t n) {
  if(n >= strip->numLEDs) return 0; // Out of bounds, return no color.
  uint8_t *p;
  if(strip->wOffset == strip->rOffset) { // Is RGB-type device
    p = &strip->pixels[n * 3];
    if(strip->brightness) {
      // Stored color was decimated by setBrightness(). Returned value
      // attempts to scale back to an approximation of the original 24-bit
      // value used when setting the pixel color, but there will always be
      // some error -- those bits are simply gone. Issue is most
      // pronounced at low brightness levels.
      return (((uint32_t)(p[strip->rOffset] << 8) / strip->brightness) << 16) |
             (((uint32_t)(p[strip->gOffset] << 8) / strip->brightness) <<  8) |
             ( (uint32_t)(p[strip->bOffset] << 8) / strip->brightness       );
    } else {
      // No brightness adjustment has been made -- return 'raw' color
      return ((uint32_t)p[strip->rOffset] << 16) |
             ((uint32_t)p[strip->gOffset] <<  8) |
              (uint32_t)p[strip->bOffset];
    }
  } else {                 // Is RGBW-type device
    p = &strip->pixels[n * 4];
    if(strip->brightness) { // Return scaled color
      return (((uint32_t)(p[strip->wOffset] << 8) / strip->brightness) << 24) |
             (((uint32_t)(p[strip->rOffset] << 8) / strip->brightness) << 16) |
             (((uint32_t)(p[strip->gOffset] << 8) / strip->brightness) <<  8) |
             ( (uint32_t)(p[strip->bOffset] << 8) / strip->brightness       );
    } else { // Return raw color
      return ((uint32_t)p[strip->wOffset] << 24) |
             ((uint32_t)p[strip->rOffset] << 16) |
             ((uint32_t)p[strip->gOffset] <<  8) |
              (uint32_t)p[strip->bOffset];
    }
  }
}
uint8_t anp_sine8(uint8_t x) {
  return _NeoPixelSineTable[x];
}
uint8_t anp_gamma8(uint8_t x) {
  return _NeoPixelGammaTable[x];
}
uint32_t anp_Color_RGB(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}
uint32_t anp_Color_RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  return ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}
uint32_t anp_ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
  uint8_t r, g, b;
  if(hue < 510) {         // Red to Green-1
    b = 0;
    if(hue < 255) {       //   Red to Yellow-1
      r = 255;
      g = hue;            //     g = 0 to 254
    } else {              //   Yellow to Green-1
      r = 510 - hue;      //     r = 255 to 1
      g = 255;
    }
  } else if(hue < 1020) { // Green to Blue-1
    r = 0;
    if(hue <  765) {      //   Green to Cyan-1
      g = 255;
      b = hue - 510;      //     b = 0 to 254
    } else {              //   Cyan to Blue-1
      g = 1020 - hue;     //     g = 255 to 1
      b = 255;
    }
  } else if(hue < 1530) { // Blue to Red-1
    g = 0;
    if(hue < 1275) {      //   Blue to Magenta-1
      r = hue - 1020;     //     r = 0 to 254
      b = 255;
    } else {              //   Magenta to Red-1
      r = 255;
      b = 1530 - hue;     //     b = 255 to 1
    }
  } else {                // Last 0.5 Red (quicker than % operator)
    r = 255;
    g = b = 0;
  }
  uint32_t v1 =   1 + val; // 1 to 256; allows >>8 instead of /255
  uint16_t s1 =   1 + sat; // 1 to 256; same reason
  uint8_t  s2 = 255 - sat; // 255 to 0
  return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
          (((((g * s1) >> 8) + s2) * v1) & 0xff00)       |
         ( ((((b * s1) >> 8) + s2) * v1)           >> 8);
}
uint32_t anp_gamma32(uint32_t x) {
  uint8_t *y = (uint8_t *)&x;
  for(uint8_t i=0; i<4; i++)
    y[i] = anp_gamma8(y[i]);
  return x; // Packed 32-bit return
}

#ifdef __cplusplus
}
#endif
