// This is a mash-up of the Due show() code + insights from Michael Miller's
// ESP8266 work for the NeoPixelBus library: github.com/Makuna/NeoPixelBus
// Needs to be a separate .c file to enforce IRAM_ATTR execution.

#if defined(ESP8266)

#ifdef __cplusplus
extern "C" {
#endif

// #include <Arduino.h>
#ifdef ESP8266
#include "esp_attr.h"
#include "esp8266/eagle_soc.h"
#include "esp8266/gpio_register.h"
#include "driver/gpio.h"
#include <stdbool.h>
#endif

static uint32_t _getCycleCount(void) __attribute__((always_inline));
static inline uint32_t _getCycleCount(void) {
  uint32_t ccount;
  __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
  return ccount;
}

#ifdef ESP8266
void IRAM_ATTR anp_espShow(
 uint8_t pin, uint8_t *pixels, uint32_t numBytes, bool is800KHz) {
#else
void espShow(
 uint8_t pin, uint8_t *pixels, uint32_t numBytes, bool is800KHz) {
#endif

#define CYCLES_800_T0H  (F_CPU / 2500000) // 0.4us
#define CYCLES_800_T1H  (F_CPU / 1250000) // 0.8us
#define CYCLES_800      (F_CPU /  800000) // 1.25us per bit
#define CYCLES_400_T0H  (F_CPU / 2000000) // 0.5uS
#define CYCLES_400_T1H  (F_CPU /  833333) // 1.2us
#define CYCLES_400      (F_CPU /  400000) // 2.5us per bit

  uint8_t *p, *end, pix, mask;
  uint32_t t, time0, time1, period, c, startTime;

#ifdef ESP8266
  uint32_t pinMask;
  pinMask   = BIT(pin);
#endif

  p         =  pixels;
  end       =  p + numBytes;
  pix       = *p++;
  mask      = 0x80;
  startTime = 0;

#ifdef NEO_KHZ400
  if(is800KHz) {
#endif
    time0  = CYCLES_800_T0H;
    time1  = CYCLES_800_T1H;
    period = CYCLES_800;
#ifdef NEO_KHZ400
  } else { // 400 KHz bitstream
    time0  = CYCLES_400_T0H;
    time1  = CYCLES_400_T1H;
    period = CYCLES_400;
  }
#endif

  for(t = time0;; t = time0) {
    if(pix & mask) t = time1;                             // Bit high duration
    while(((c = _getCycleCount()) - startTime) < period); // Wait for bit start
#ifdef ESP8266
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);       // Set high
#else
    gpio_set_level(pin, 1);
#endif
    startTime = c;                                        // Save start time
    while(((c = _getCycleCount()) - startTime) < t);      // Wait high duration
#ifdef ESP8266
    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);       // Set low
#else
    gpio_set_level(pin, 0);
#endif
    if(!(mask >>= 1)) {                                   // Next bit/byte
      if(p >= end) break;
      pix  = *p++;
      mask = 0x80;
    }
  }
  while((_getCycleCount() - startTime) < period); // Wait for last bit
}

#ifdef __cplusplus
}
#endif

#endif // ESP8266
