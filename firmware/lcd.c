// LiquidCrystal_I2C V2.0

#include "lcd.h"
#include "twi.h"
#include <inttypes.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "bootloaderconfig.h"

uint8_t _addr = I2C_LCD_ADDR;
uint8_t _displayfunction;
uint8_t _displaycontrol;
uint8_t _displaymode;
uint8_t _numlines = 2;
uint8_t _backlightval = LCD_BACKLIGHT;

void LCD_init()
{
	TWI_init();
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS | LCD_2LINE;

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	_delay_us(50000);

	// Now we pull both RS and R/W low to begin commands
	LCD_expanderWrite(_backlightval);// reset expanderand turn backlight off (Bit 8 =1)
	_delay_ms(1000);

	// we start in 8bit mode, try to set 4 bit mode
	LCD_write4bits(0x03);
	_delay_us(4500); // wait min 4.1ms
	LCD_write4bits(0x03);
	_delay_us(4500); // wait min 4.1ms
	LCD_write4bits(0x03);
	_delay_us(150);

	// finally, set to 4-bit interface
	LCD_write4bits(0x02);

	// set # lines, font size, etc.
	LCD_command(LCD_FUNCTIONSET | _displayfunction);

	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	LCD_display();

	// clear it off
	LCD_clear();

	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	LCD_command(LCD_ENTRYMODESET | _displaymode);

	LCD_home();
}

/********** high level commands, for the user! */
void LCD_clear()
{
	LCD_command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
	_delay_us(2000);  // this command takes a long time!
}

void LCD_home()
{
	LCD_command(LCD_RETURNHOME);  // set cursor position to zero
	_delay_us(2000);  // this command takes a long time!
}

void LCD_setCursor(uint8_t col, uint8_t row)
{
	int row_offsets[] =
	{ 0x00, 0x40, 0x14, 0x54 };
	if (row > _numlines)
	{
		row = _numlines - 1;    // we count rows starting w/0
	}
	LCD_command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void LCD_noDisplay()
{
	_displaycontrol &= ~LCD_DISPLAYON;
	LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_display()
{
	_displaycontrol |= LCD_DISPLAYON;
	LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LCD_noCursor()
{
	_displaycontrol &= ~LCD_CURSORON;
	LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_cursor()
{
	_displaycontrol |= LCD_CURSORON;
	LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LCD_noBlink()
{
	_displaycontrol &= ~LCD_BLINKON;
	LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_blink()
{
	_displaycontrol |= LCD_BLINKON;
	LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn the (optional) backlight off/on
void LCD_noBacklight(void)
{
	_backlightval = LCD_NOBACKLIGHT;
	LCD_expanderWrite(0);
}

void LCD_backlight(void)
{
	_backlightval = LCD_BACKLIGHT;
	LCD_expanderWrite(0);
}

/*********** mid level commands, for sending data/cmds */

inline void LCD_command(uint8_t value)
{
	LCD_send(value, 0);
}

void LCD_write(char value)
{
	LCD_send(value, Rs);
}

void LCD_writeStr(char * str)
{
	uint8_t i = 0;
	for(;str[i] != 0 && i < 16; i++)
	{
		LCD_write(str[i]);
	}
}

/************ low level data pushing commands **********/

// write either command or data
void LCD_send(uint8_t value, uint8_t mode)
{
	uint8_t highnib = value >> 4;
	uint8_t lownib = value & 0x0F;
	LCD_write4bits((highnib) | mode);
	LCD_write4bits((lownib) | mode);
}

void LCD_write4bits(uint8_t value)
{
	LCD_expanderWrite(value);
	LCD_pulseEnable(value);
}

void LCD_expanderWrite(uint8_t _data)
{
	uint8_t s = _data | _backlightval;
	TWI_send(_addr, &s, 1);
}

void LCD_pulseEnable(uint8_t _data)
{
	LCD_expanderWrite(_data | En);	// En high
	_delay_us(1);		// enable pulse must be >450ns

	LCD_expanderWrite(_data & ~En);	// En low
	_delay_us(50);		// commands need > 37us to settle
}

