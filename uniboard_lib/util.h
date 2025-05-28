
#ifndef UTIL_H
#define UTIL_H

#include "onewire_library.h"

void put_pixel(PIO pio, uint sm, uint32_t pixel_grb);

uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
uint32_t urgb_b_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t br);
uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max);


void play_tone(uint gpio, uint32_t freq, uint32_t duration_ms);
void tone_on(uint gpio, uint32_t freq);
void tone_off(uint gpio);


uint64_t tempinit(OW ow, PIO pio, uint gpio);
int16_t readTemp(OW ow, PIO pio, uint gpio, uint64_t romcode);
  
void pirates();


void uniboard_gpio_init();
uint8_t getSwitches();
uint16_t readPot(uint8_t adcPin);


void mouse_move(int8_t dx, int8_t dy);

void send_hid_report(uint8_t report_id, uint32_t btn);

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len);

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);


#endif