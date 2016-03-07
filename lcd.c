/******************************************************************************/
/*Files to Include                                                            */
/******************************************************************************/

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#endif

#include <stdio.h>
#include "system.h"
#include "user.h"
#include "lcd.h"

void lcd_wait_busy() {
    wait_1ms();
    wait_1ms();
}

void lcd_send_nibble(char n) {
    lcd_data.data = n;
    _delay(1);
    lcd_ctrl.enable = 1;
    _delay(1);
    lcd_ctrl.enable = 0;
}

void lcd_send_byte(char address, char n) {
    lcd_ctrl.rs = 0;
    __delay_us(48);
    lcd_ctrl.rs = address;
    _delay(1);
    lcd_send_nibble(n >> 4); //byte superior
    lcd_send_nibble(n & 0xf); //byte inferior
}

void lcd_write_cmd(unsigned char dat) {
    lcd_send_byte(0, dat);
}

void lcd_write_dat(char dat) {
    unsigned char t;
    lcd_send_byte(1, dat);
}

void lcd_print_at(char* text, unsigned char x, unsigned char y) {
    unsigned char cmd;
    cmd = 0x80 + x;
    if (y != 0) cmd = cmd + 0x40;
    lcd_write_cmd(cmd); // Goto XY location (x,y)
    while (*text != 0) {
        lcd_write_dat(*text++);
        CLRWDT();
    }
}

void lcd_print_line(char* text, unsigned char line) {
    unsigned char x;
    x = 0;
    if (line == 0) lcd_write_cmd(0x80);
    else lcd_write_cmd(0xc0);
    lcd_wait_busy();
    while (*text != 0) {
        lcd_write_dat(*text++);
        x++;
    }
    while (x++ < 16) {
        lcd_write_dat(' ');
    }
}

void lcd_init(void) {
    char i, j;
    lcd_ctrl.rs = 0;
    lcd_ctrl.enable = 0;
    for (i = 0; i != 15; ++i)
        wait_1ms();
    for (i = 1; i != 3; ++i) {
        lcd_send_nibble(3);
        for (j = 0; j <= 5; ++j)
            wait_1ms();
    }
    lcd_send_nibble(2);
    for (i = 0; i <= 3; ++i)
        lcd_send_byte(0, LCD_INIT_STRING[i]);
}
