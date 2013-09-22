// LiquidCrystal_I2C V2.0

#ifndef LCD_h
#define LCD_h

#include <inttypes.h>
#include <avr/pgmspace.h>

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x00
#define LCD_NOBACKLIGHT 0x80

#define En 0b00010000  // Enable bit
#define Rw 0b00100000  // Read/Write bit
#define Rs 0b01000000  // Register select bit
void LCD_init();
void LCD_clear();
void LCD_home();
void LCD_setCursor(uint8_t, uint8_t);
void LCD_noDisplay();
void LCD_display();
void LCD_noCursor();
void LCD_cursor();
void LCD_noBlink();
void LCD_blink();
void LCD_noBacklight();
void LCD_backlight();
void LCD_command(uint8_t);
void LCD_write(char);
void LCD_writeStr(char *);
void LCD_send(uint8_t, uint8_t);
void LCD_write4bits(uint8_t);
void LCD_expanderWrite(uint8_t);
void LCD_pulseEnable(uint8_t);

#endif
