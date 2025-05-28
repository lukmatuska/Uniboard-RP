
#ifndef LCD_1602_I2C_H
#define LCD_1602_I2C_H


// commands


// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0

#define MAX_LINES      2
#define MAX_CHARS      16


/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val);
void lcd_toggle_enable(uint8_t val);
// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode);
void lcd_clear(void);
// go to location on LCD
void lcd_set_cursor(int line, int position);
static inline void lcd_char(char val);
void lcd_string(char *s);
void lcd_init();

#endif