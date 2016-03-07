/******************************************************************************/
/*Files to Include                                                            */
/******************************************************************************/

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
#include <htc.h>        /* HiTech General Include File */
#endif

#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#include "user.h"

/******************************************************************************/
/* Interrupt Routines                                                         */
/******************************************************************************/

/* Baseline devices don't have interrupts. Note that some PIC16's 
 * are baseline devices.  Unfortunately the baseline detection macro is 
 * _PIC12 */

extern volatile unsigned int timer0count;
extern volatile unsigned int timercount80;
extern volatile bit timer0flag;
extern unsigned char powercount;
extern unsigned char power_heat1, power_heat2;
extern volatile bit secflag;
extern volatile bit Buzzer_stat;

//extern volatile bit ti_busy;
extern volatile bit rx_valid;
extern volatile char rx_char;

#ifdef _18F2620

void __interrupt isr(void) {
#else

void interrupt isr(void) {
#endif
    /* This code stub shows general interrupt handling.  Note that these
    conditional statements are not handled within 3 seperate if blocks.
    Do not use a seperate if block for each interrupt flag to avoid run
    time errors. */

    //timer0handler
    if (TMR0IE && TMR0IF) {
        TMR0 = 0xFF - RT0Reload - TMR0;
        if (timer0count-- == 0) {
            timer0flag = 1;
            timer0count = T0COUNTVAL;
        }
        if (timercount80-- == 0) {
            // 50 times a second (will be replaced by int. handler for INT0
            // Keypress handler
            //  LED2=~LED2;
            NOP();
            if (++powercount > 49) {
                powercount = 0;
                secflag = 1;
            }
            if (power_heat1 > powercount) {
                HEAT1 = 1; //LED1=0;
            } else {
                HEAT1 = 0; //LED1=1;
            }
            if (power_heat2 > powercount) {
                HEAT2 = 1; //LED2=0;
            } else {
                HEAT2 = 0; //LED2=1;
            }
            timercount80 = TCOUNTVAL;
        }
        if (!Buzzer_stat)
            BUZZER = 0;
        else
            BUZZER = ~BUZZER;
        TMR0IF = 0; //clear this interrrupt
    } else if (RCIE && RCIF) {
        if (!rx_valid)//do not overwrite a message
            rx_char = RCREG;
        rx_valid = 1;
        RCIF = 0; //clear this interrrupt
    }
}
