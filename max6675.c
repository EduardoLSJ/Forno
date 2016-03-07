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
#include "user.h"
#include "max6675.h"

#ifndef SPI_SCK

#define SPI_SCK LATAbits.LATA6
#define SPI_NCS LATCbits.LATC0
#define SPI_SO LATEbits.LATE3

#endif

unsigned int read_temperature(void) {
    unsigned int shiftreg = 0;
    unsigned char count;
    static bit thermocouple_open;
    SPI_SO = 1; // SO is input
    SPI_SCK = 0; // clock is low, needed for first time use
    SPI_NCS = 0; // Select the max6675 and stop conversion
    for (count = 0; count < 13; count++) {
        SPI_SCK = 1;
        shiftreg = shiftreg << 1;
        if (SPI_SO == 1) shiftreg = shiftreg + 1;
        SPI_SCK = 0;
    }
    thermocouple_open = 0;
    SPI_SCK = 1;
    if (SPI_SO == 1)
        thermocouple_open = 1; //thermocouple input is open
    SPI_SCK = 0;
    for (count = 0; count < 2; count++) // disregard the last 2 bytes
    {
        SPI_SCK = 1;
        SPI_SCK = 0;
    }
    SPI_NCS = 1; // Start next conversion
    CLRWDT();
    return shiftreg;
}
