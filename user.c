/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/
#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
#include <htc.h>        /* HiTech General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#include "system.h"
#include "user.h"
#include "keys.h"
#include "lcd.h"
#include "rs_232.h"
#include "24c1024.h"
#include "max6675.h"


/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/* <Initialize variables in user.h and insert code for user algorithms.> */

void InitApp(void) {
    /* Initialize peripherals */
    init_controller();
    init_rs232();
    init_keys();

    // Initialize interrupts
    di();
    PEIE = 1;
    RCIE = 1; // Enable serialport interrupt
    //    TXIE = 1;
    TMR0IE = 1; // Enable timer0 interrupt

    /* Enable interrupts */
    ei(); // Enable interrupts global

    lcd_init();
    I2C_init();
    BT_CMD = 0;
    BT_reset = 0;
    wait_1ms();
    BT_reset = 1;
}

void init_controller(void) {
    /* Setup analog functionality and port direction */
    HEAT_port = 0;

#ifdef _18F2620

    PORTA = 0;
    LATA = 0;
    ADCON0 = 0;
    ADCON1 = 0x0F;
    CMCON = 0x07;

    TRISA = TRISA_CONF;
    TRISB = TRISB_CONF;
    TRISC = TRISC_CONF;

    //port_b_pullups(TRUE)
    RBPU = 0;
    //buzzer nivel baixo
    Buzzer_stat = 0;

    // Initialize Timer 0
    TMR0 = T0COUNTVAL; // Timer 0 : 1 interrupt each 500uSec
    T08BIT = 1; //Timer0 is configured as an 8-bit timer/counter
    T0CS = 0; //Internal instruction cycle clock (CLKO)
    PSA = 0;
    T0CONbits.T0PS = RT0Prescaler; //prescaler = 4
    TMR0ON = 1; //Enables Timer0

#else

    //turn off adc ports -> all digital
    ADCON0 = 0;
    ADCON1 = 0x06;

    TRISA = TRISA_CONF;
    TRISB = TRISB_CONF;
    TRISC = TRISC_CONF;

    // Initialize Timer 0
    TMR0 = T0COUNTVAL; // Timer 0 : 1 interrupt each 500uSec
    OPTION_REGbits.nRBPU = 0; //RBPU enable
    OPTION_REGbits.T0CS = 0; //RTCC internal
    OPTION_REGbits.PSA = 0; //Prescaler to WDT
    OPTION_REGbits.PS = RT0Prescaler; //prescaler = 4

#endif

    // Initialize Timer 0 int handler
    timer0count = T0COUNTVAL;
    timer0flag = 0;
    timercount80 = TCOUNTVAL;
    secflag = 0;

    //   __delay_us(25);
}

void wait_1ms(void) {
    unsigned int end_point;
    static bit EA_temp;
    EA_temp = GIE;
    GIE = 0; // Disable interrupts loading end_point is
    //a multi-instruction operation on a volatile
    // variable, therefore this must be done atomic
    end_point = timer0count;
    GIE = EA_temp; // Restore global interrupt enable
    if (end_point >= 2) // Set end_point to 1,5mS later.
        //This ensures that the delay is between 1mS and 1,5mS
        end_point = end_point - 2;
    else
        end_point = end_point + (T0COUNTVAL - 2);
    while (timer0count != end_point) {
        CLRWDT();
    };
}

void secbeat(void) {
    while (secflag != 1) {
        CLRWDT();
    };
    secflag = 0;
}

void calibrate(void) {
    signed int tmp;
    unsigned char phase;
    signed int deltaT, deltaT100;
    sprintf(str_buf, mensagens_calibrate[ESFRIANDO]);
    lcd_print_line(str_buf, 0);
    phase = 0; // assume we have to cool down first
    deltaT = 0; // Start from scratch
    while (phase < 3) {
        tmp = read_temperature();
        if ((phase == 0) && (tmp < (50 * 4))) {
            phase = 1;
            sprintf(str_buf, mensagens_calibrate[AQUECENDO]);
            lcd_print_line(str_buf, 0);
            power_heat1 = 50;
            power_heat2 = 50;
        }
        if ((phase == 1) && (tmp > (100 * 4))) {
            phase = 2;
            sprintf(str_buf, mensagens_calibrate[SOBRETEMP]);
            lcd_print_line(str_buf, 0);
            power_heat1 = 0;
            power_heat2 = 0;
            deltaT100 = deltaT; // remember the deltaT at 100 C
        }
        if ((phase == 2) && (deltaT < 0)) {
            phase = 3;
            sprintf(str_buf, mensagens_calibrate[PRONTO]);
            lcd_print_line(str_buf, 0);
            oven_overshoot = ((long) (((long) tmp * 25)-(10000)) / deltaT100);
            sprintf(str_buf, "sobretemp:%3.3u%cC", oven_overshoot, 0xdf);
            lcd_print_line(str_buf, 1);
            while ((keys_get() & KEY_ENTER) == 0) {
            };
            write_byte_24c1024(0x00, oven_overshoot);
            write_byte_24c1024(0x01, ~oven_overshoot);
        }
        sprintf(str_buf, "T:%3.3u%cC d:%d.%2.2d%cC", (tmp / 4), 0xdf, deltaT
                / 100, abs((deltaT % 100)), 0xdf);
        lcd_print_line(str_buf, 1);

        while (secflag != 1) {//aguarda 1 segundo entre iterações
            if ((keys_get() & KEY_ENTER) && (phase != 3))//encerra a pedido
                return;
            wait_1ms();
        };
        secflag = 0;
        deltaT = ((3 * deltaT)+(((int) (read_temperature()) - tmp)*25)) / 4;
        // read_temp() returns temperature * 4. *25 results in
        // degrees Celsius * 100;
    }
}

void controle() {
    unsigned char phase, keys;
    signed int tmp, des_tmp, des_slope, prev_tmp, deltaT, tempsoak;
    unsigned int timer;
    unsigned int EEPROM_adr;
    static bit refresh;

    sprintf(str_buf, mensagens_controle[PRE_RES]);
    lcd_print_line(str_buf, 0);
    phase = 0;
    EEPROM_adr = LOG_START_ADR; // pointer to where the

    while (phase < 10) {
        refresh = 0;
        while (secflag == 0) {// wait for end of second...
            keys = keys_get();
            if (keys & KEY_ENTER) // Stop the cycle
            {
                power_heat1 = 0;
                power_heat2 = 0;
                phase = 10;
                return;
            }
            // Adjust desired temperature. In the case of SOAK-phase
            // adjust the endtemperature, because the desired temperature
            // is calculated every second.
            if (keys & KEY_RIGHT) {//increment e atualiza
                if (phase == 3) tempsoak++;
                else des_tmp++;
                refresh = 1;
                break;
            }
            if (keys & KEY_LEFT) {//decrementa e atualiza
                if (phase == 3) tempsoak--;
                else des_tmp--;
                refresh = 1;
                break;
            }
            wait_1ms();
        }
        if (!refresh)
            tmp = read_temperature();
        switch (phase) {
            case 0:
                des_tmp = 50;
                if (tmp < 50 * 4) {
                    phase = 1;
                    des_tmp = perfil.profile.pre_endtemp;
                    des_slope = perfil.profile.pre_slope;
                    //adicionado buzzer para avisar mudança
                    Buzzer_stat = 1;
                    for (keys = 50; keys != 0; keys--)
                        wait_1ms();
                    Buzzer_stat = 0;
                }
                break;
            case 1: // PREHEAT
                sprintf(str_buf, "PRE-AQUEC %u%cC", des_tmp, 0xdf);
                lcd_print_line(str_buf, 0);
                if ((tmp / 4) >= des_tmp) {
                    phase = 2; // proceed to preaqu_time-phase;
                    tempsoak = perfil.profile.pre_endtemp;
                    timer = perfil.profile.pre_time;
                    //adicionado buzzer para avisar mudança
                    Buzzer_stat = 1;
                    for (keys = 50; keys != 0; keys--)
                        wait_1ms();
                    Buzzer_stat = 0;
                }
                break;
            case 2: //PREHEAT time
                // Display remaining time
                sprintf(str_buf, " %2.2u:%2.2u", timer / 60, timer % 60);
                lcd_print_at(str_buf, 10, 0);
                if (timer == 0) {
                    phase = 3; // proceed to soak-phase;
                    timer = perfil.profile.soak_time;
                    sprintf(str_buf, mensagens_controle[SOAK]);
                    lcd_print_line(str_buf, 0);
                    tempsoak = perfil.profile.soak_endtemp;
                    //adicionado buzzer para avisar mudança
                    Buzzer_stat = 1;
                    for (keys = 50; keys != 0; keys--)
                        wait_1ms();
                    Buzzer_stat = 0;
                }
                break;
            case 3: // SOAK
                // Display remaining time
                sprintf(str_buf, " %2.2u:%2.2u", timer / 60, timer % 60);
                lcd_print_at(str_buf, 10, 0);
                // Calculate the desired temperature.
                //Sliding scale from pre_endtemp to soak_endtemp
                des_tmp = (((perfil.profile.soak_time - timer) * tempsoak)+
                        (timer * perfil.profile.pre_endtemp)) / perfil.profile.soak_time;
                if (timer == 0) {
                    phase = 4; // proceed to dwell-phase;
                    des_tmp = perfil.profile.reflow_peaktemp;
                    sprintf(str_buf, mensagens_controle[DWELL]);
                    lcd_print_line(str_buf, 0);
                    //adicionado buzzer para avisar mudança
                    Buzzer_stat = 1;
                    for (keys = 50; keys != 0; keys--)
                        wait_1ms();
                    Buzzer_stat = 0;
                }
                break;
            case 4: // DWELL
                if (tmp / 4 > (des_tmp - 5)) {
                    phase = 5; // proceed to reflow-phase
                    sprintf(str_buf, mensagens_controle[REFLUXO]);
                    lcd_print_line(str_buf, 0);
                    timer = perfil.profile.reflow_time;
                    //adicionado buzzer para avisar mudança
                    Buzzer_stat = 1;
                    for (keys = 50; keys != 0; keys--)
                        wait_1ms();
                    Buzzer_stat = 0;
                }
                break;
            case 5: // REFLOW
                // Display remaining time
                sprintf(str_buf, " %2.2u:%2.2u", timer / 60, timer % 60);
                lcd_print_at(str_buf, 10, 0);
                if (timer == 0) {
                    phase = 6; // proceed to cooldown-phase
                    des_tmp = 50;
                    sprintf(str_buf, mensagens_controle[RESFRIANDO]);
                    lcd_print_line(str_buf, 0);
                    //adicionado buzzer para avisar mudança
                    Buzzer_stat = 1;
                    for (keys = 50; keys != 0; keys--)
                        wait_1ms();
                    Buzzer_stat = 0;
                }
                break;
            case 6: // COOLDOWN
                //aviso sonoro até "abrir" a tampa do forno
                if ((tmp / 4) >= (perfil.profile.reflow_peaktemp - 10)) {
                    Buzzer_stat = 1;
                    for (keys = 50; keys != 0; keys--)
                        wait_1ms();
                    Buzzer_stat = 0;
                }
                if ((tmp / 4) < des_tmp) {
                    phase = 10; // DONE!
                    des_tmp = 0;
                }
                break;
            default:
                break;
        }
        if (!refresh) {
            deltaT = ((3 * deltaT)+((tmp - prev_tmp)*25)) / 4;
            prev_tmp = tmp;
            tmp = tmp / 4; // convert tmp to oC
        }
        if ((phase == 0) || (phase == 6)||(phase == 10)) {// During cooldown the elements must
            // be inactive
            power_heat1 = 0;
            power_heat2 = 0;
        } else {
            if (tmp < des_tmp) {
                if (tmp + ((oven_overshoot * deltaT) / 100) < des_tmp) {
                    power_heat1 = 50;
                    power_heat2 = 50;
                } else {
                    power_heat1 = 0;
                    power_heat2 = 0;
                }
            } else {
                if (tmp + ((oven_overshoot * deltaT) / 50) < des_tmp) {
                    power_heat1 = 25;
                    power_heat2 = 25;
                } else {
                    power_heat1 = 0;
                    power_heat2 = 0;
                }
            }
        }
        if (phase == 3)
            sprintf(str_buf, "T%3.3d%cC end:%3.3d%cC", tmp, 0xdf, tempsoak,
                0xdf);
        else
            sprintf(str_buf, "T%3.3d%cC to:%3.3d%cC", tmp, 0xdf, des_tmp, 0xdf);

        lcd_print_line(str_buf, 1);
        sprintf(str_buf, "%u,%d,%d,%d,%d\n\r", phase, tmp, des_tmp,
                power_heat1, power_heat2);
        printstr_rs232(str_buf);

        if (!refresh) {
            if (tmp_log) {
                write_byte_24c1024(EEPROM_adr, (tmp & 0x00ff));
                EEPROM_adr++;
                write_byte_24c1024(EEPROM_adr, (tmp >> 8));
                EEPROM_adr++;
            }
            secflag = 0;
            timer--;
        }
    }
    if (tmp_log) {//grava encerrante do registro
        write_byte_24c1024(EEPROM_adr, 0);
        EEPROM_adr++;
        write_byte_24c1024(EEPROM_adr, 0);
    }
}

void edit(void) {
    short int contador;
    unsigned char state, event;
    unsigned char param, keys;//, prev_param;
    unsigned int value;//, prev_value, min_value, max_value;
    static bit done, change_param, change_state, edicao_state;

    const unsigned int LISTA_max_value [] = {
        max_editar,
        max_pre_slope,
        max_pre_endtemp,
        max_pre_time,
        max_soak_time,
        max_soak_endtemp,
        max_reflow_time,
        max_reflow_peaktemp,
    };

    const unsigned int LISTA_min_value [] = {
        min_editar,
        min_pre_slope,
        min_pre_endtemp,
        min_pre_time,
        min_soak_time,
        min_soak_endtemp,
        min_reflow_time,
        min_reflow_peaktemp,
    };

    change_state = 0; /*muda estado*/
    change_param = 0; /*muda valor de parametro*/
    edicao_state = 0; /*edição do parametro*/
    done = 0;
    event = ev_right;
    state = st_editar;

    while (done == 0) {
     
        switch (state) {
            case st_editar:
                /*para cada tecla, gera uma condição de eventos*/
                switch (event) {
                    case ev_enter:
                        done = 1;
                        event = ev_none;
                        break;
                    case ev_left:
                        state = st_reflow_peaktemp;
                        change_state = 1;
                        break;
                    case ev_right:
                        state = st_pre_slope;
                        change_state = 1;
                        break;
                    default:
                        break;
                }
                break;
            case st_pre_slope:
                switch (event) {
                    case ev_left:
                        if (edicao_state) {
                            value--;
                            change_param = 1;
                        } else {
                            state = st_editar;
                            change_state = 1;
                        }
                        break;
                    case ev_right:
                        if (edicao_state) {
                            value++;
                            change_param = 1;
                        } else {
                            state = st_pre_endtemp;
                            change_state = 1;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case st_pre_endtemp:
                switch (event) {
                    case ev_left:
                        if (edicao_state) {
                            value--;
                            change_param = 1;
                        } else {
                            state = st_pre_slope;
                            change_state = 1;
                        }
                        break;
                    case ev_right:
                        if (edicao_state) {
                            value++;
                            change_param = 1;
                        } else {
                            state = st_pre_time;
                            change_state = 1;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case st_pre_time:
                switch (event) {
                    case ev_left:
                        if (edicao_state) {
                            value--;
                            change_param = 1;
                        } else {
                            state = st_pre_endtemp;
                            change_state = 1;
                        }
                        break;
                    case ev_right:
                        if (edicao_state) {
                            value++;
                            change_param = 1;
                        } else {
                            state = st_soak_time;
                            change_state = 1;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case st_soak_time:
                switch (event) {
                    case ev_left:
                        if (edicao_state) {
                            value--;
                            change_param = 1;
                        } else {
                            state = st_pre_time;
                            change_state = 1;
                        }
                        break;
                    case ev_right:
                        if (edicao_state) {
                            value++;
                            change_param = 1;
                        } else {
                            state = st_soak_endtemp;
                            change_state = 1;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case st_soak_endtemp:
                switch (event) {
                    case ev_left:
                        if (edicao_state) {
                            value--;
                            change_param = 1;
                        } else {
                            state = st_soak_time;
                            change_state = 1;
                        }
                        break;
                    case ev_right:
                        if (edicao_state) {
                            value++;
                            change_param = 1;
                        } else {
                            state = st_reflow_time;
                            change_state = 1;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case st_reflow_time:
                switch (event) {
                    case ev_left:
                        if (edicao_state) {
                            value--;
                            change_param = 1;
                        } else {
                            state = st_soak_endtemp;
                            change_state = 1;
                        }
                        break;
                    case ev_right:
                        if (edicao_state) {
                            value++;
                            change_param = 1;
                        } else {
                            state = st_reflow_peaktemp;
                            change_state = 1;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case st_reflow_peaktemp:
                switch (event) {
                    case ev_left:
                        if (edicao_state) {
                            value--;
                            change_param = 1;
                        } else {
                            state = st_reflow_time;
                            change_state = 1;
                        }
                        break;
                    case ev_right:
                        if (edicao_state) {
                            value++;
                            change_param = 1;
                        } else {
                            state = st_editar;
                            change_state = 1;
                        }
                        break;
                    default:
                        break;
                }
                break;
        }

        if (event == ev_enter){
            edicao_state = ~edicao_state;
            if (edicao_state){
                lcd_write_cmd(13); //blink cursor
            }
            else
                lcd_write_cmd(12); //no blink cursor
            
            perfil.param.indice[state] = value;
            contador = 450;
        }

        if (change_state) {
            value = perfil.param.indice[state];

            sprintf(str_buf, str_line_0[state]);
            lcd_print_line(str_buf, 0);
            contador = 450;
            change_param = 1; /*desenhar a linha 1*/
        }
        if (change_param) {
            if (value <= LISTA_min_value [state])
                value = LISTA_min_value [state];
            if (value >= LISTA_max_value [state])
                value = LISTA_max_value [state];
            perfil.param.indice[state] = value;
            if (state == st_pre_time || state == st_soak_time){
                sprintf(str_buf, str_line_1[state], value / 60,
                    value % 60);
                contador = 80;
            }
            else{
                sprintf(str_buf, str_line_1[state], value);
                contador = 150;
            }

            lcd_print_line(str_buf, 1);
            if(change_state)
                contador = 450;
        }
        for (; contador != 0; contador--)
            wait_1ms();
        change_state = 0;
        change_param = 0;
        event = ev_none;
        do {
            keys = keys_get();
        } while (!keys);

        /* função de monitoração de teclas*/
        if (keys & KEY_ENTER) event = ev_enter;
        else if (keys & KEY_RIGHT) event = ev_right;
        else if (keys & KEY_LEFT) event = ev_left;
    }

    sprintf(str_buf, mensagens_edit[Salva]);
    lcd_print_line(str_buf, 0);
    sprintf(str_buf, mensagens_edit[sim]);
    lcd_print_line(str_buf, 1);
    while (1) {
        keys = keys_get();
        p_prof = (unsigned char*) &perfil.profile;
                       
        if (keys & KEY_RIGHT) {
            for (param = 0; param<sizeof (perfil.profile); param++)
                write_byte_24c1024(param + PROFILES_START_ADR,
                    *(unsigned char*) p_prof++); // stores EEPROM
            return;
        }
        else if (keys & KEY_LEFT){
            read_buffer_24c1024(PROFILES_START_ADR,
            (unsigned char*) &perfil.profile, sizeof (perfil.profile));
            return;
        }
    }
}


    void send_log(void) {
        unsigned int adr, dat;
        unsigned char keys;
        sprintf(str_buf, mensagens_send_log[Enviando]);
        lcd_print_line(str_buf, 0);
        sprintf(str_buf, mensagens_send_log[velocidade]);
        lcd_print_line(str_buf, 1);
        adr = LOG_START_ADR;
        dat = 1;
        while ((dat != 0)&(adr < LOG_END_ADR)) {
            keys = keys_get();
            if (keys & KEY_ENTER) return;
            dat = read_byte_24c1024(adr)+(read_byte_24c1024(adr + 1) << 8);
            adr++;
            adr++;
            sprintf(str_buf, "%u\n\r", dat);
            printstr_rs232(str_buf);
        }
    }

    void logging(void) {
        unsigned char keys, change;
        keys = 0;
        change = 1;
        sprintf(str_buf, mensagens_logging[tecle]);
        lcd_print_line(str_buf, 0);
        while ((keys & KEY_ENTER) == 0) {
            keys = keys_get();
            if (keys & KEY_LEFT) {
                tmp_log = !tmp_log;
                change = 1;
            }
            if (keys & KEY_RIGHT) {
                send_log();
                return;
            }
            if (change) {
                change = 0;
                if (tmp_log){
                    sprintf(str_buf, mensagens_logging[Reg_on]);
                    lcd_print_line(str_buf, 1);
                }
                else{
                    sprintf(str_buf, mensagens_logging[Reg_off]);
                    lcd_print_line(str_buf, 1);
                }
            }
            wait_1ms();
        }
    }
