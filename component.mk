#
# Component Makefile
#

COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_SRCDIRS := src

CPPFLAGS += -DESP8266
CPPFLAGS += -DF_CPU=CONFIG_ESP8266_DEFAULT_CPU_FREQ_MHZ*1000000
CPPFLAGS += -I'$(IDF_PATH)/components/esp8266/include'
CPPFLAGS += -I'$(IDF_PATH)/components/freertos/include'
CPPFLAGS += -I'$(IDF_PATH)/components/freertos/port/esp8266/include'
