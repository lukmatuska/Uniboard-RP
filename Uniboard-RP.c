
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"


#include "uniboard_def.h"
#include "tone_to_freq.h"
#include "onewire_library.h"    // onewire library functions
#include "ds18b20.h"    
#include "ow_rom.h"  



// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

// flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0

#define MAX_LINES      2
#define MAX_CHARS      16


/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val) {
#ifdef i2c_default
    i2c_write_blocking(i2c_default, addr, &val, 1, false);
#endif
}
/*
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
*/
void lcd_toggle_enable(uint8_t val) {
    // Toggle enable pin on LCD display
    // We cannot do this too quickly or things don't work
#define DELAY_US 600
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position) {
    int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    lcd_send_byte(val, LCD_COMMAND);
}

static inline void lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s) {
    while (*s) {
        lcd_char(*s++);
    }
}

void lcd_init() {
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);

    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd_clear();
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

void __not_in_flash_func(adc_capture)(uint16_t *buf, size_t count) {
    adc_fifo_setup(true, false, 0, false, false);
    adc_run(true);
    for (size_t i = 0; i < count; i = i + 1)
        buf[i] = adc_fifo_get_blocking();
    adc_run(false);
    adc_fifo_drain();
}



void readTemp(OW ow, PIO pio, uint gpio) {
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
                    sprintf (text, "temp%d:%f", i, temp / 16.0);
                    lcd_set_cursor(0, 0);
                    lcd_string(text);

                    
                }
                putchar ('\n');
            }
            
        }

    }

}



int main()
{
    stdio_init_all();
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(I2C_SDA_PIN, I2C_SCL_PIN, GPIO_FUNC_I2C));

    lcd_init();
    adc_init();
    gpio_set_function(POT1, GPIO_FUNC_SIO);
    gpio_disable_pulls(POT1);
    gpio_set_input_enabled(POT1, false);
    adc_select_input(POT1_ADC);



    PIO pio = pio0;
    uint gpio = ONEWIRE_PIN;
    OW ow;
    uint offset;
    /*
    if (pio_can_add_program (pio, &onewire_program)) {
        offset = pio_add_program (pio, &onewire_program);

        // claim a state machine and initialise a driver instance
        if (ow_init (&ow, pio, offset, gpio)) {

            // find and display 64-bit device addresses
            int maxdevs = 10;
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
                for (int i = 0; i < num_devs; i += 1) {
                    ow_reset (&ow);
                    ow_send (&ow, OW_MATCH_ROM);
                    for (int b = 0; b < 64; b += 8) {
                        ow_send (&ow, romcode[i] >> b);
                    }
                    ow_send (&ow, DS18B20_READ_SCRATCHPAD);
                    int16_t temp = 0;
                    temp = ow_read (&ow) | (ow_read (&ow) << 8);
                    char text[20];
                    sprintf (text, "temp%d:%f", i, temp / 16.0);
                    lcd_set_cursor(0, 0);
                    lcd_string(text);

                    
                }
                putchar ('\n');
            }
            
        } else {
            lcd_set_cursor(0, 0);
        lcd_string("could not initialize driver");
        }
    } else {
        lcd_set_cursor(0, 0);
        lcd_string("could not add the program");
    }*/


    

    while (true) {
        lcd_set_cursor(0, 0);
        char text[20];
        uint32_t val = adc_read();
        sprintf (text, "pot%d   ", map(val, 0, 4096, 0, 255));
        lcd_string(text);
        sleep_ms(10);
        //lcd_clear();
    }
}
