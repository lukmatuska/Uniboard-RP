

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"


#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "pico/binary_info.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"


#include "uniboard_def.h"
#include "util.h"
#include "onewire_library.h" 
#include "tone_to_freq.h"
#include "ds18b20.h"  
#include "ow_rom.h"




PIO pio_led;
uint sm;
uint offset_led;


    PIO pio_led;
    uint sm;
    uint offset_led;


void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}


uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}
uint32_t urgb_b_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t br) {
    return
            ((uint32_t) ((r*br)/255) << 8) |
            ((uint32_t) ((g*br)/255) << 16) |
            (uint32_t) ((b*br)/255);
}

uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void play_tone(uint gpio, uint32_t freq, uint32_t duration_ms) { //  for buzzer
    // Set GPIO function to PWM
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    // Get PWM slice
    uint slice_num = pwm_gpio_to_slice_num(gpio);

    // Calculate wrap and clock divider
    uint32_t clock = 1000000; // 125 MHz default
    uint32_t wrap = (uint32_t)(clock / freq + 0.5f); // rounded

    pwm_set_clkdiv(slice_num, 125);

    // Set PWM period (wrap)
    pwm_set_wrap(slice_num, wrap);

    // Set 50% duty cycle
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), wrap / 2);

    // Enable PWM output
    pwm_set_enabled(slice_num, true);

    // Wait for duration
    sleep_ms(duration_ms);

    // Disable PWM output
    pwm_set_enabled(slice_num, false);
}

void tone_on(uint gpio, uint32_t freq) { //  for buzzer
    // Set GPIO function to PWM

    // Get PWM slice
    uint slice_num = pwm_gpio_to_slice_num(gpio);

    // Calculate wrap and clock divider
    uint32_t clock = 1000000; // 125 MHz default
    uint32_t wrap = (uint32_t)(clock / freq + 0.5f); // rounded

    pwm_set_clkdiv(slice_num, 125);

    // Set PWM period (wrap)
    pwm_set_wrap(slice_num, wrap);

    // Set 50% duty cycle
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(gpio), wrap / 2);

    // Enable PWM output
    pwm_set_enabled(slice_num, true);
}

void tone_off(uint gpio) { //  for buzzer 
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_enabled(slice_num, false);
}



uint64_t tempinit(OW ow, PIO pio, uint gpio){
    uint offset;
    if (pio_can_add_program (pio, &onewire_program)) {
        offset = pio_add_program (pio, &onewire_program);

        if (ow_init (&ow, pio, offset, gpio)) {

            // find and display 64-bit device addresses
            int maxdevs = 3;
            uint64_t romcode[maxdevs];
            int num_devs = ow_romsearch (&ow, romcode, maxdevs, OW_SEARCH_ROM);
            return romcode[0];
            
        }

    }

}

int16_t readTemp(OW ow, PIO pio, uint gpio, uint64_t romcode) {
    uint offset;
    if (pio_can_add_program (pio, &onewire_program)) {
        offset = pio_add_program (pio, &onewire_program);

        if (ow_init (&ow, pio, offset, gpio)) {

            // find and display 64-bit device addresses
            int maxdevs = 3;
            uint64_t romcode[maxdevs];
            int num_devs = ow_romsearch (&ow, romcode, maxdevs, OW_SEARCH_ROM);

            printf("Found %d devices\n", num_devs);      
            for (int i = 0; i < num_devs; i += 1) {
                printf("\t%d: 0x%llx\n", i, romcode[i]);
            }
            putchar ('\n');

            while (num_devs > 0) {
                printf("Cycle: %d\n", num_devs);
                // start temperature conversion in parallel on all devices
                // (see ds18b20 datasheet)
                ow_reset (&ow);
                ow_send (&ow, OW_SKIP_ROM);
                ow_send (&ow, DS18B20_CONVERT_T);

                // wait for the conversions to finish
                while (ow_read(&ow) == 0);

                // read the result from each device
                for (int i = 0; i < num_devs; i++) {
                    ow_reset (&ow);
                    ow_send (&ow, OW_MATCH_ROM);
                    for (int b = 0; b < 64; b += 8) {
                        ow_send (&ow, romcode[i] >> b);
                    }
                    ow_send (&ow, DS18B20_READ_SCRATCHPAD);
                    int16_t temp = 0;
                    temp = ow_read (&ow) | (ow_read (&ow) << 8);
                    char text[20];
                    return temp;
                    //sprintf (text, "temp%d:%f", i, temp / 16.0);
                    //lcd_set_cursor(0, 0);
                    //lcd_string(text);

                    
                }
                putchar ('\n');
            }
            
        }

    }

}

    
void pirates(){
    play_tone(BUZZER_PIN, A4, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, C5, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, D5, 100);
    sleep_ms(200);
    play_tone(BUZZER_PIN, D5, 100);
    sleep_ms(200);
    
    play_tone(BUZZER_PIN, D5, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, E5, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, F5, 100);
    sleep_ms(200);
    play_tone(BUZZER_PIN, F5, 100);
    sleep_ms(200);

    play_tone(BUZZER_PIN, F5, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, G5, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, E5, 100);
    sleep_ms(200);
    play_tone(BUZZER_PIN, E5, 100);
    sleep_ms(200);

    play_tone(BUZZER_PIN, D5, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, C5, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, C5, 100);
    sleep_ms(50);
    play_tone(BUZZER_PIN, D5, 100);
    sleep_ms(350);
}

void uniboard_gpio_init() {
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(I2C_SDA_PIN, I2C_SCL_PIN, GPIO_FUNC_I2C));


    adc_init();
    gpio_set_function(POT0, GPIO_FUNC_SIO);
    gpio_set_function(POT1, GPIO_FUNC_SIO);
    gpio_set_function(POT2, GPIO_FUNC_SIO);
    gpio_disable_pulls(POT0);
    gpio_disable_pulls(POT1);
    gpio_disable_pulls(POT2);
    gpio_set_input_enabled(POT0, false);
    gpio_set_input_enabled(POT1, false);
    gpio_set_input_enabled(POT2, false);


    gpio_init(SW0);
    gpio_init(SW1);
    gpio_init(SW2);
    gpio_init(SW3);
    gpio_init(SW4);
    gpio_init(SW5);
    gpio_init(SW6);
    gpio_init(SW7);

    gpio_init(LED0);
    gpio_init(LED1);
    gpio_init(LED2);
    gpio_init(LED3);

    gpio_set_dir(SW0, GPIO_IN);
    gpio_set_dir(SW1, GPIO_IN);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_set_dir(SW3, GPIO_IN);
    gpio_set_dir(SW4, GPIO_IN);
    gpio_set_dir(SW5, GPIO_IN);
    gpio_set_dir(SW6, GPIO_IN);
    gpio_set_dir(SW7, GPIO_IN);

    gpio_set_dir(LED0, GPIO_OUT);
    gpio_set_dir(LED1, GPIO_OUT);
    gpio_set_dir(LED2, GPIO_OUT);
    gpio_set_dir(LED3, GPIO_OUT);

    gpio_set_function(POUT0, GPIO_FUNC_PWM);
    gpio_set_function(POUT1, GPIO_FUNC_PWM);
    gpio_set_function(POUT2, GPIO_FUNC_PWM);
    gpio_set_function(POUT3, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    

}
uint8_t getSwitches(){
    return
    ( gpio_get(SW0) ) |
    ( gpio_get(SW1) << 1) |
    ( gpio_get(SW2) << 2) |
    ( gpio_get(SW3) << 3) |
    ( gpio_get(SW4) << 4) |
    ( gpio_get(SW5) << 5) |
    ( gpio_get(SW6) << 6) |
    ( gpio_get(SW7) << 7) ;
}
uint16_t readPot(uint8_t adcPin){
    adc_select_input(adcPin);
    return adc_read();
}




//--------------------------------------------------------------------+
// USB HID fron the example
//--------------------------------------------------------------------+

void mouse_move(int8_t dx, int8_t dy){
    if ( !tud_hid_ready() ) return;

    tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, dx, dy, 0, 0);

}

void send_hid_report(uint8_t report_id, uint32_t btn)
{
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;

  switch(report_id)
  {
    case REPORT_ID_KEYBOARD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      bool has_keyboard_key = false;

      if ( btn )
      {
        uint8_t keycode[6] = { 0 };
        keycode[0] = HID_KEY_A;

        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
        has_keyboard_key = true;
      }else
      {
        // send empty key report if previously has key pressed
        if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
      }
    }
    break;

    case REPORT_ID_MOUSE:
    {
      int8_t const delta = 5;

      // no button, right + down, no scroll, no pan
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta, delta, 0, 0);
    }
    break;

    case REPORT_ID_CONSUMER_CONTROL:
    {
      // use to avoid send multiple consecutive zero report
      bool has_consumer_key = false;

      if ( btn )
      {
        // volume down
        uint16_t volume_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
        has_consumer_key = true;
      }else
      {
        // send empty key report (release key) if previously has key pressed
        uint16_t empty_key = 0;
        if (has_consumer_key) tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        has_consumer_key = false;
      }
    }
    break;

    case REPORT_ID_GAMEPAD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      bool has_gamepad_key = false;

      hid_gamepad_report_t report =
      {
        .x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
        .hat = 0, .buttons = 0
      };

      if ( btn )
      {
        report.hat = GAMEPAD_HAT_UP;
        report.buttons = GAMEPAD_BUTTON_A;
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));

        has_gamepad_key = true;
      }else
      {
        report.hat = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;
        if (has_gamepad_key) tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_gamepad_key = false;
      }
    }
    break;

    default: break;
  }
}


// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;

  uint8_t next_report_id = report[0] + 1u;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, board_button_read());
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be (at least) 1
      if ( bufsize < 1 ) return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on

      }else
      {
        // Caplocks Off: back to normal blink

      }
    }
  }
}
