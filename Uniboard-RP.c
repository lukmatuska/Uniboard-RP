
#define PLATFORM_RP2040_V2

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "uniboard_def.h"
#include "util.h"
#include "tone_to_freq.h"
#include "onewire_library.h"    // onewire library functions
#include "ds18b20.h"    
#include "ow_rom.h"  
#include "lcd_1602_i2c.h"


uint8_t old_switches = 0;




void handleSwitches(uint8_t switches){
    if(switches == old_switches) {
        return;
    }
    old_switches = switches;
    if(switches & (1 << 0)){
            tone_on(BUZZER_PIN, B4);
            gpio_put(LED0, 1);
        } else {
            
            gpio_put(LED0, 0);
        }
        if(switches & (1 << 1)){
            tone_on(BUZZER_PIN, C5);
            gpio_put(LED1, 1);
        } else {
            
            gpio_put(LED1, 0);
        }
        if(switches & (1 << 2)){
            tone_on(BUZZER_PIN, D5);
            gpio_put(LED2, 1);
        } else {
           
            gpio_put(LED2, 0);
        }
        if(switches & (1 << 3)){
            tone_on(BUZZER_PIN, G5);
            gpio_put(LED3, 1);
        } else {            
            gpio_put(LED3, 0);
        }

        if((switches & 0b1111) == 0){
            tone_off(BUZZER_PIN);
        }
}


// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(int8_t dx, int8_t dy)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  // Remote wakeup
  if ( tud_suspended() && btn )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    //send_hid_report(REPORT_ID_MOUSE, btn);
    mouse_move(dx, dy);
  }
}


int main()
{

    PIO pio_temp = pio0;
    uint gpio = ONEWIRE_PIN;
    OW ow;
    uint offset_temp;
    

    PIO pio_led;
    uint sm;
    uint offset_led;

    uint32_t trigtime = time_us_32() + 100000;

    uint16_t partRed = 0;
    uint16_t partGreen = 0;
    uint16_t partBlue = 0;
    uint16_t brightness = 16;
    int8_t mouse_dx = 0;
    int8_t mouse_dy = 0;
    char text[20];

    stdio_init_all();
    uniboard_gpio_init();
    tud_init(BOARD_TUD_RHPORT);

     if (board_init_after_tusb) {
        board_init_after_tusb();
     }
    lcd_init();

    partRed = map(readPot(POT0_ADC), 0, 4096, 255, 0);
    partGreen = map(readPot(POT1_ADC), 0, 4096, 255, 0);
    partBlue = map(readPot(POT2_ADC), 0, 4096, 255, 0);


    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio_led, &sm, &offset_led, LED_RGB_PIN, 1, true);
    ws2812_program_init(pio_led, sm, offset_led, LED_RGB_PIN, 800000, 0);

    uint64_t temp_romcode = tempinit(ow, pio_temp, ONEWIRE_PIN);

    while (true) {

        uint8_t switches = getSwitches();

        handleSwitches(switches);
        

        

        if(switches & (1 << 4)){
                brightness = map(readPot(POT1_ADC), 0, 4096, 255, 0);
        }
        if(switches & (1 << 5)){
                partRed = map(readPot(POT0_ADC), 0, 4096, 255, 0);
                partGreen = map(readPot(POT1_ADC), 0, 4096, 255, 0);
                partBlue = map(readPot(POT2_ADC), 0, 4096, 255, 0);
                
        }
        if(switches & (1 << 6)){
            mouse_dx = map(readPot(POT0_ADC), 0, 4096, 16, -16);
            mouse_dy = map(readPot(POT1_ADC), 0, 4096, -16, 16);
            tud_task(); // tinyusb device task
            hid_task(mouse_dx, mouse_dy);
        } else {
            mouse_dx = 0;
            mouse_dy = 0;
        }

            
            
            //sprintf (text, "%d   ", temp_romcode);
        put_pixel(pio_led, sm, urgb_b_u32(partRed, partGreen, partBlue, brightness));
        put_pixel(pio_led, sm, urgb_b_u32(partRed, partGreen, partBlue, brightness));
        put_pixel(pio_led, sm, urgb_b_u32(partRed, partGreen, partBlue, brightness));

        uint32_t micros = time_us_32();
        if(!(micros % 25)){
            lcd_set_cursor(0, 0);
            lcd_clear();
            sprintf (text, "x:%3d, y:%3d", mouse_dx, mouse_dy);
            lcd_string(text);
        }



        
    }
}
