#ifndef _LCD_H_
#define _LCD_H_

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
#include <htc.h>        /* HiTech General Include File */
#endif

//typedef union  {

struct lcd_d { // This structure is overlayed
    unsigned data : 4; // on to an I/O port to gain
    // low order up.
};

#ifdef _18F2620
volatile struct lcd_d lcd_data @ &LATA;
#else
volatile struct lcd_d lcd_data @ &PORTA;
#endif

struct lcd_c {
    unsigned unused : 4;
    unsigned enable : 1;
    unsigned rs : 1;
};
#ifdef _18F2620
volatile struct lcd_c lcd_ctrl @ &LATC;
#else
volatile struct lcd_c lcd_ctrl @ &PORTC;
#endif




#define lcd_type 2           // 0=5x7, 1=5x10, 2=2 lines
#define lcd_line_two 0x40    // LCD RAM address for the second line


unsigned char const LCD_INIT_STRING[4] = {0x20 | (lcd_type << 2), 0xc, 1, 6};
// These bytes need to be sent to the LCD
// to start it up.


// The following are used for setting
// the I/O port direction register.

void lcd_init(void);
void lcd_print_at(char* text, unsigned char x, unsigned char y);
void lcd_print_line(char* text, unsigned char line);
void lcd_write_cmd(unsigned char dat);

#endif
