/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
#include <htc.h>        /* HiTech General Include File */
#endif

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "lcd.h"
#include "max6675.h"
#include "24c1024.h"
#include "keys.h"
#include "rs_232.h"


/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/

/* i.e. uint8_t <variable_name>; */
volatile unsigned int timer0count;
volatile unsigned int timercount80;
volatile bit timer0flag;
volatile bit secflag;
bit tmp_log;
volatile bit Buzzer_stat;

char str_buf[20];

char menu_item, prev_menu_item;
unsigned char power_heat1, power_heat2;
// power_heatx stores the current powersetting for heatelement x
// range (0 .. 50)
unsigned char powercount; // used in int_handler

union Uperfis perfil;
//struct Tprofile profile;
unsigned char* p_prof;
unsigned char oven_overshoot; // overshoot in oC at dT/dt = 1 oC/s

/******************************************************************************/
/* Main Program                                                               */

/******************************************************************************/
void main(void) {
    unsigned char keys;
    static bit disp_choice;
    unsigned char t_hour = 15,t_min=45; /*TODO criar struct em RTC.h*/

#ifdef _18F2620
    /* Configure the oscillator for the device */
    ConfigureOscillator();
#endif

    /* Initialize I/O and Peripherals for application */
    InitApp();

    CLRWDT();
    power_heat1 = 0;
    power_heat2 = 0; // No heatelement activated
    menu_item = 0;

    sprintf(str_buf, mensagens_main[Forno]);
    printstr_rs232(str_buf);//"Forno SMD v1.0\n\r");
    sprintf(str_buf, mensagens_main[Forno1]);
    lcd_print_line(str_buf, 0);//"Forno SMD v1.0", 0);
    sprintf(str_buf, mensagens_main[Inicializando]);
    lcd_print_line(str_buf, 1);//"  Inicializando", 1);
    Buzzer_stat = 1;
    for (keys = 50; keys != 0; keys--)
        wait_1ms();
    Buzzer_stat = 0;

    sprintf(str_buf, mensagens_main[Forno_SMD]);
    printstr_rs232(str_buf);//"\n\r\n\rForno SMD\n\r");
    if (is_24c1024_present() == 0) {
        sprintf(str_buf, mensagens_main[FALHA]);
        lcd_print_line(str_buf, 0);//"FALHA DA EEPROM!", 0);
        sprintf(str_buf, mensagens_main[troque]);
        lcd_print_line(str_buf, 1);//"troque o 24c1024", 1);
        while ((keys_get() & KEY_ENTER) == 0) {
        }
    };

    tmp_log = 0; // Switch off the log-function to preserve EEPROM
    oven_overshoot = read_byte_24c1024(0x00); // Read the calibration-value
    keys = ~read_byte_24c1024(0x01);
    if ( oven_overshoot != keys){ // Check if it is ok.
        sprintf(str_buf, mensagens_main[Calibracao]);
        lcd_print_line(str_buf, 0);//"Calibracao", 0); //Else start the calibration-procedure
        sprintf(str_buf, mensagens_main[ENT]);
        lcd_print_line(str_buf, 1);//"[ENT] inicia", 1);
        while ((keys_get() & KEY_ENTER) == 0) {
        }
        calibrate();
    }
    read_buffer_24c1024(PROFILES_START_ADR,
            (unsigned char*) &perfil.profile, sizeof (perfil.profile));
    disp_choice = 1;
    sprintf(str_buf, mensagens_main[DINST]);
    lcd_print_line(str_buf, 0);//"DINST   SMD-OVEN", 0);
    menu_item = MENU_MIN;

    while (true) {
        keys = keys_get(); // get key-status
        if (keys & KEY_RIGHT) ++menu_item;
        if (keys & KEY_LEFT) --menu_item;
        if (menu_item > MENU_MAX) menu_item = MENU_MIN;
        if (menu_item < MENU_MIN) menu_item = MENU_MAX;
        if (keys != 0) disp_choice = 1;

        if (disp_choice != 0)
            switch (menu_item) {
                case MENU_START:
                    sprintf(str_buf, mensagens_main[Iniciar]);
                    lcd_print_line(str_buf, 1);//"->Iniciar Solda", 1);
                    break;
                case MENU_EDIT:
                    sprintf(str_buf, mensagens_main[Editar]);
                    lcd_print_line(str_buf, 1);//"->Editar Perfil", 1);
                    break;
                case MENU_LOG:
                    sprintf(str_buf, mensagens_main[Registrar]);
                    lcd_print_line(str_buf, 1);//"->Registrar", 1);
                    break;
                case MENU_CAL:
                    sprintf(str_buf, mensagens_main[Calibrar]);
                    lcd_print_line(str_buf, 1);//"->Calibrar", 1);
                    break;
                default: menu_item = MENU_MIN;
            }
        if (keys & KEY_ENTER) {
            switch (menu_item) {
                case MENU_START:
                    controle();
                    break;
                case MENU_CAL:
                    calibrate();
                    break;
                case MENU_EDIT:
                    edit();
                    break;
                case MENU_LOG:
                    logging();
                    break;
                default:
                    break;
            }
            sprintf(str_buf, mensagens_main[Terminado]);
            lcd_print_line(str_buf, 0);//"Terminado", 0);
            sprintf(str_buf, mensagens_main[press]);
            lcd_print_line(str_buf, 1); //"press [ENT]", 1);
            while ((keys_get() & KEY_ENTER) == 0) {
                power_heat1 = 0;
                power_heat2 = 0; // No heatelement activated
            };
            //            lcd_print_line("DINST   SMD-OVEN", 0);
            disp_choice = 1;
        }
        //            read_time; /*TODO criar rotina de leitura RTC

        if (secflag) {
            sprintf(str_buf, mensagens_main[DINST1], t_hour, t_min);//"DINST     %2uh%2um", t_hour, t_min);
            lcd_print_line(str_buf, 0);
            secflag = 0;
        }
        wait_1ms();
        power_heat1 = 0;
        power_heat2 = 0; // No heatelement activated
    }
}
