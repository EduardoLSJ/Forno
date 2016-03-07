/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
#include <htc.h>        /* HiTech General Include File */
#endif

//#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include "user.h"
#include "rs_232.h"
#include "system.h"

//volatile bit ti_busy;
volatile bit rx_valid;
volatile char rx_char;

#define USART_BRG ((long)SYS_FREQ / (16 * (long)BAUDRATE)) - 1

#if USART_BITS == 8
#define USART_B 0
#endif

#if USART_PARITY == N
#define USART_P 1
#endif

#if USART_STOP == 1
#define USART_S 1
#endif

//#define USART_TXSTA USART_B << 6 | 0x24
//#define USART_RCSTA 0x90 | USART_B << 6
#define USART_TXSTA 0x26
#define USART_RCSTA 0x90


void init_rs232(void) {

    // Initialize baudgenerator for serial port
    SPBRG = USART_BRG;
    // Configure UART
    TXSTA = USART_TXSTA;
    RCSTA = USART_RCSTA;

//    ti_busy = 0;
    rx_valid = 0;
}

void printchar_rs232(char ch) {
    while (TXIF == 0) {
        CLRWDT();
    }; // Wait until last transmission is completed
//    ti_busy = 1;
    TXREG = ch; // start a new transmission
}

void printstr_rs232(char* str) {
    while (*str != 0) // Repeat until end of string
    {
        while (TXIF == 0) {
            CLRWDT();
        }; // Wait for the last transmission to complete
//        ti_busy = 1;
        TXREG = *(str++); // transmit a new character and increase the pointer
    }
}

unsigned char getchar_rs232(void) {
    if (rx_valid) {
        rx_valid = 0;
        CLRWDT();
        return rx_char;
    } else return 0;
}
