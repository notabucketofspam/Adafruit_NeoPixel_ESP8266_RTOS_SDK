#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "anp_component.h"

#ifdef __cplusplus
  extern "C" {
    void app_main(void);
  }
#endif

#define PIN 4
#define NUMPIXELS 16
#define DELAYVAL 500

AnpStrip *pixels;

void app_main(void) {
  pixels = new_AnpStrip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
  anp_begin(pixels);
  while (1) {
    anp_clear(pixels);
    uint16_t pixel_index;
    for (pixel_index = 0; pixel_index < NUMPIXELS; ++pixel_index) {
      anp_setPixelColor_C(pixels, pixel_index, anp_Color_RGB(0, 150, 0));
      anp_show(pixels);
      vTaskDelay(DELAYVAL / portTICK_PERIOD_MS);
    }
  }
}
