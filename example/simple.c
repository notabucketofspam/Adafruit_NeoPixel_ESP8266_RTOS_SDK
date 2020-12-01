#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "anp_component.h"

#ifdef __cplusplus
  extern "C" {
    void app_main(void);
  }
#endif

#define ANP_PIN 4
#define ANP_PIXEL_COUNT 30

AnpStrip *strip;

void app_main(void) {
  strip = new_AnpStrip(ANP_PIXEL_COUNT, ANP_PIN, NEO_GRB + NEO_KHZ800);
  anp_begin(strip);
  ESP_LOGI(__ESP_FILE__, "app_main ok");
  uint32_t counter = 0;
  while (1) {
    anp_clear(strip);
    uint16_t pixel_index;
    for (pixel_index = 0; pixel_index < ANP_PIXEL_COUNT; ++pixel_index) {
      anp_setPixelColor_C(strip, pixel_index, anp_Color_RGB(0, 150, 0));
      anp_show(strip);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(__ESP_FILE__, "Loop counter: %ud", ++counter);
  }
}
